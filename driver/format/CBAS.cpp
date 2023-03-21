#include "driver/format/CBAS.h"
#include "driver/cJSON.h"
#include "driver/utils/resize_without_initialization.h"

CBASResultSet::CBASResultSet(
    const std::string & timezone, AmortizedIStreamReader & stream, std::unique_ptr<ResultMutator> && mutator, CallbackCookie & cbCookie)
    : cookie(cbCookie), ResultSet(stream, std::move(mutator)) {
        
    std::map<std::string, std::string> cb_to_ch_types;
    cb_to_ch_types["string?"] = "String";
    cb_to_ch_types["double?"] = "Float64";
    cb_to_ch_types["int64?"] = "Int64";

    cJSON * json;
    json = cJSON_Parse(cookie.queryMeta.c_str());
    if (json && json->type == cJSON_Object) {
        cJSON * signature = cJSON_GetObjectItem(json, "signature");
        if (signature && signature->type == cJSON_Object) {
            cJSON * names = cJSON_GetObjectItem(signature, "name");
            if (names && names->type == cJSON_Array) {
                std::int32_t columns_count = cJSON_GetArraySize(names);
                columns_info.resize(columns_count);

                cJSON * name = names->child;
                int cIdx = 0;
                while (name) {
                    if (name->type == cJSON_String) {
                        columns_info[cIdx].name = name->valuestring;
                    }
                    name = name->next;
                    cIdx++;
                }
            }

            cJSON * types = cJSON_GetObjectItem(signature, "type");
            if (types && types->type == cJSON_Array) {
                std::int32_t columns_count = cJSON_GetArraySize(types);
                columns_info.resize(columns_count);

                cJSON * type = types->child;
                int cIdx = 0;
                while (type) {
                    if (type->type == cJSON_String) {
                        if (cb_to_ch_types.find(type->valuestring) != cb_to_ch_types.end()) {
                            columns_info[cIdx].type = cb_to_ch_types[type->valuestring];
                        } else {
                            //todo:
                            columns_info[cIdx].type = "String";
                        }


                        TypeParser parser {columns_info[cIdx].type};
                        TypeAst ast;

                        if (parser.parse(&ast)) {
                            columns_info[cIdx].assignTypeInfo(ast, timezone);

                            if (convertUnparametrizedTypeNameToTypeId(columns_info[cIdx].type_without_parameters)
                                == DataSourceTypeId::Unknown) {
                                // Interpret all unknown types as String.
                                columns_info[cIdx].type_without_parameters = "String";
                            }
                        } else {
                            // Interpret all unparsable types as String.
                            columns_info[cIdx].type_without_parameters = "String";
                        }
                        columns_info[cIdx].updateTypeInfo();
                    }


                    type = type->next;
                    cIdx++;
                }
            }
        }
    }

    finished = columns_info.empty();
}

void readStrValue(std::string & src, DataSourceType<DataSourceTypeId::String> & dest) {
    dest.value = std::move(src);
}

bool CBASResultSet::readNextRow(Row & row) {
    std::string doc;
    if (std::getline(cookie.queryResultStrm, doc)) {
        cJSON * json;
        json = cJSON_Parse(doc.c_str());
        if (json && json->type == cJSON_Object) {
            cJSON * col = json->child;
            int cIdx = 0;
            while (col) {
                std::ostringstream oss;
                if (col->type == cJSON_String) {
                    oss << col->valuestring;
                } else if (col->type == cJSON_Number) {
                    oss << cJSON_Print(col);
                }

                std::string colVal = oss.str();

                auto & dest = row.fields[cIdx];
                auto & column_info = columns_info[cIdx];

                // todo: // make it similar to ODBCDRiver2.cpp (handling more types)
                // readValue(row.fields[cIdx], columns_info[cIdx]);

                auto value = string_pool.get();
                value_manip::to_null(value);

                //todo: handle null case
                // bool is_null = false;

                resize_without_initialization(value, colVal.size());

                value = colVal;
                if (column_info.display_size_so_far < value.size())
                    column_info.display_size_so_far = value.size();

                DataSourceType<DataSourceTypeId::String> value2;
                readStrValue(value, value2);

                dest.data = std::move(value2);
                if (value.capacity() > initial_string_capacity_g)
                    string_pool.put(std::move(value));


                col = col->next;
                cIdx++;
            }
        }
        return true;
    }

    return false;
}

CBASResultReader::CBASResultReader(
    const std::string & timezone_, std::istream & raw_stream, std::unique_ptr<ResultMutator> && mutator, CallbackCookie & cbCookie)
    : ResultReader(timezone_, raw_stream, std::move(mutator)) {
    result_set = std::make_unique<CBASResultSet>(timezone, stream, releaseMutator(), cbCookie);
}

bool CBASResultReader::advanceToNextResultSet() {
    if (result_set) {
        result_mutator = result_set->releaseMutator();
        result_set.reset();
    }

    return hasResultSet();
}
