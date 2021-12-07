#pragma once

#include <stddef.h>
#include <stdint.h>
#include <atomic>

#include "mio/parallelism/aba_ptr.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T, size_t DEL_CHACHE>
        class ref_ptr;

        template <typename T, size_t DEL_CHACHE>
        class hazard_ptr
        {
        private:
            friend class ref_ptr<T, DEL_CHACHE>;

            struct node
            {
                T *ptr;
                std::atomic<node *> next;
                std::atomic<aba_ptr<node>> free;
                std::atomic<bool> flag;
            };

            std::atomic<T *> ptr_;

            node ref_list_;

            node del_list_;
            std::atomic<uint64_t> del_count_ = 0;

            std::atomic_flag del_flag_ = false;

            void list_push(node &list, node *n)
            {
                node *exp = list.next;
                do
                {
                    n->next = exp;
                } while (!list.next.compare_exchange_weak(exp, n));
            }

            aba_ptr<node> alloc(node &list)
            {
                aba_ptr<node> exp = list.free;
                aba_ptr<node> ret;
                do
                {
                    //没有可以节点了 就自己分配一个
                    if (exp == nullptr)
                    {
                        ret = new node;
                        ret->flag = true;
                        ret->ptr = nullptr;
                        list_push(list, ret);
                        return ret;
                    }

                    ret = exp;
                } while (!list.free.compare_exchange_weak(exp, exp->free));

                ret->flag = true;
                return ret;
            }

            void free(node &list, aba_ptr<node> n)
            {
                n->ptr = nullptr;
                n->flag = false;
                aba_ptr<node> exp = list.free;
                do
                {
                    n->free = exp;
                } while (!list.free.compare_exchange_weak(exp, n));
            }

            //返回一个 ref node
            node *make_ref_node()
            {
                //尝试寻找一个释放的节点
                node *ret = alloc(ref_list_);

                do
                {
                    ret->ptr = ptr_;
                } while (ret->ptr != ptr_);

                return ret;
            }

            //删除该指针
            void del_ptr(T *ptr)
            {
                //尝试从 free list获取一个
                node *del = alloc(del_list_);
                del->ptr = ptr;

                ++del_count_;

                if (del_count_ > DEL_CHACHE && !del_flag_.test_and_set())
                {
                    del_count_ = 0;
                    reclaim();
                    del_flag_.clear();
                }
            }

            //释放节点
            void reclaim()
            {
                //存储所有 正在使用的指针
                std::unordered_set<T *> ref_set;

                for (node *n = ref_list_.next; n; n = n->next)
                {
                    ref_set.insert(n->ptr);
                }

                for (node *n = del_list_.next; n; n = n->next)
                {
                    if (ref_set.find(n->ptr) == ref_set.end() && n->flag != false)
                    {
                        delete n->ptr;
                        free(del_list_, n);
                    }
                    else
                    {
                        ++del_count_;
                    }
                }
            }

        public:
            hazard_ptr()
            {
                ptr_ = nullptr;

                ref_list_.next = nullptr;
                ref_list_.free = nullptr;

                del_list_.next = nullptr;
                del_list_.free = nullptr;
            }

            ~hazard_ptr()
            {
                //释放 ptr
                delete ptr_;

                //释放所有未释放节点
                reclaim();

                //清除所有node

                for (node *n = ref_list_.next; n;)
                {
                    node *p = n;
                    n = n->next;
                    delete p;
                }

                for (node *n = del_list_.next; n;)
                {
                    node *p = n;
                    n = n->next;
                    delete p;
                }
            }

            hazard_ptr &operator=(T *ptr)
            {
                auto old_ptr = ptr_.exchange(ptr);
                del_ptr(old_ptr);
                return *this;
            }
        };

        template <typename T, size_t DEL_CHACHE = 100>
        class ref_ptr
        {
        public:
            using hazard_ptr_t = hazard_ptr<T, DEL_CHACHE>;
            using node_t = typename hazard_ptr<T, DEL_CHACHE>::node;

        private:
            hazard_ptr_t *hazard_ptr_;
            node_t *ref_node_;

        public:
            ref_ptr()
            {
                hazard_ptr_ = nullptr;
                ref_node_ = nullptr;
            }

            ref_ptr(hazard_ptr_t &hazard_ptr)
            {
                hazard_ptr_ = &hazard_ptr;
                ref_node_ = hazard_ptr_->make_ref_node();
            }

            ~ref_ptr()
            {
                //把节点放到free list中
                if (ref_node_)
                {
                    hazard_ptr_->free(hazard_ptr_->ref_list_, ref_node_);
                }
            }

            T *get()
            {
                return ref_node_->ptr;
            }

            T &operator*()
            {
                return *get();
            }

            T *operator->()
            {
                return get();
            }
        };

    } // namespace parallelism
} // namespace mio