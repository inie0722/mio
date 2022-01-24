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
        Json::Value csv_load(Stream &&stream, const std::string &index)
        {
            std::string line;
            std::getline(stream, line);

            std::vector<std::string> head;
            boost::split(head, line, boost::is_any_of(","));

            size_t ind = std::find(head.begin(), head.end(), index) - head.begin();
            Json::Value value;
            for (int i = 0; std::getline(stream, line); i++)
            {
                std::vector<std::string> ve;
                boost::split(ve, line, boost::is_any_of(","));

                for (int ii = 0; ii < head.size(); ii++)
                {
                    value[head[ii]][ve[ind]] = ve[ii];
                }
            }

            return value;
        }

        std::string csv_dump(const Json::Value &value, const std::vector<std::string> head)
        {
            std::string ret;
            ret += boost::algorithm::join(head, ",") + "\n";

            size_t size = value[head[0]].size();
            for (auto &i : value[head[0]].getMemberNames())
            {
                for (auto &ii : head)
                {
                    ret += value[ii][i].asString() + ",";
                }

                ret.back() = '\n';
            }
            return ret;
        }
    }
}