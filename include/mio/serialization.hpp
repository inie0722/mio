#pragma once

#include <algorithm>

#include <boost/algorithm/string.hpp>

#include <json/json.h>

namespace mio
{
    namespace serialization
    {
        template <typename Stream>
        Json::Value json_load(Stream &&stream)
        {
            Json::CharReaderBuilder rbuilder;
            rbuilder["collectComments"] = false;
            JSONCPP_STRING errs;
            Json::Value value;
            Json::parseFromStream(rbuilder, stream, &value, &errs);

            return value;
        }

        std::string json_dump(const Json::Value &value)
        {
            return value.toStyledString();
        }

        template <typename Stream>
        Json::Value csv_load(Stream &&stream, const std::optional<std::string> &index)
        {
            std::string line;
            std::getline(stream, line);

            std::vector<std::string> head;
            boost::split(head, line, boost::is_any_of(","));

            Json::Value value;

            size_t ind;
            if (index != std::nullopt)
                ind = std::find(head.begin(), head.end(), *index) - head.begin();

            for (int i = 0; std::getline(stream, line); i++)
            {
                std::vector<std::string> ve;
                boost::split(ve, line, boost::is_any_of(","));

                for (int ii = 0; ii < head.size(); ii++)
                {
                    if (index != std::nullopt)
                        value[head[ii]][ve[ind]] = ve[ii];
                    else
                        value[head[ii]][i] = ve[ii];
                }
            }

            return value;
        }

        std::string csv_dump(const Json::Value &value, const std::vector<std::string> &head, const std::optional<std::string> &index)
        {
            std::string ret;
            ret += boost::algorithm::join(head, ",") + "\n";

            if (index != std::nullopt)
            {
                for (auto &i : value[*index].getMemberNames())
                {
                    for (auto &ii : head)
                    {
                        ret += value[ii][i].asString() + ",";
                    }

                    ret.back() = '\n';
                }
            }
            else
            {
                int size = value[head[0]].size();
                for (int i = 0; i < size; i++)
                {
                    for (auto &ii : head)
                    {
                        ret += value[ii][i].asString() + ",";
                    }

                    ret.back() = '\n';
                }
            }

            return ret;
        }
    }
}