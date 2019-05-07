#ifndef COLORINGCLASSIFER_SHIFT_COLORING_FILTER_H
#define COLORINGCLASSIFER_SHIFT_COLORING_FILTER_H

#include <iostream>
#include "coloring_classifier.h"
#include "utils.h"

using namespace std;

template<uint32_t bucket_num, uint32_t color_num, uint32_t class_num>
class ShiftingColoringClassifier: protected ColoringClassifier<bucket_num, color_num>
{
    typedef ColoringClassifier<bucket_num, color_num> Parent;
    static constexpr int max_offset = log2(class_num);
public:
    string name;
    constexpr static int _class_num = class_num;

    ShiftingColoringClassifier() {
        name = "CC" + string(1, char('0' + color_num));
    }

    bool build(vector<pair<uint64_t, uint32_t>> & kvs, int data_num)
    {
        Parent::clear_edge();
        for (int i = 0; i < bucket_num; i++)
            Parent::v_buckets[i].clear();

        int counters[class_num][2];
        memset(counters, 0, sizeof(counters));

        for (int i = 0; i < data_num; ++i) {
            auto & kv = kvs[i];
            uint32_t val = kv.second;
            for (int k = 0; k < max_offset; ++k) {
                counters[k][(val >> k) & 1] += 1;
            }
        }

//        for (int i = 0; i < max_offset; ++i) {
//            cout << counters[i][0] << " " << counters[i][1] << endl;
//        }
//
//        cout << "try insert..." << endl;
        for (int i = 0; i < data_num; ++i) {
            uint64_t key = kvs[i].first;
            uint32_t val = kvs[i].second;

            typename Parent::CCEdge e(key);
            Parent::pos_map.insert(make_pair(key, map<int, typename Parent::CCEdge*>()));
            Parent::neg_map.insert(make_pair(key, map<int, typename Parent::CCEdge*>()));
            for (int k = 0; k < max_offset; ++k) {
                auto p_e = new typename Parent::CCEdge(e, k);
                if ((val >> k) & 1) {
                    Parent::pos_edges.insert(p_e);
                    Parent::pos_map.find(key)->second.insert(make_pair(k, p_e));
                }
                else {
                    Parent::neg_edges.insert(p_e);
                    Parent::neg_map.find(key)->second.insert(make_pair(k, p_e));
                }
            }
        }

        return ColoringClassifier<bucket_num, color_num>::build();
    }

    vector<uint32_t> query(uint64_t key)
    {
        typename Parent::CCEdge e(key);

        uint32_t ret = 0;

        for (int k = 0; k < max_offset; ++k) {
            int c1 = Parent::get_bucket_val((e.hash_val_a + k) % bucket_num);
            int c2 = Parent::get_bucket_val((e.hash_val_b + k) % bucket_num);
            ret |= ((c1 == c2 ? 0 : 1u) << k);
        }

        return vector<uint32_t>(1, ret);
    }

    vector<uint32_t> query_multiway(uint64_t key)
    {
        return query(key);
    }

    bool insert(uint64_t item, uint32_t class_id)
    {
        bool result = true;
        typename Parent::CCEdge e(item);
        for (int k = 0; k < max_offset; ++k)
        {
            if ((class_id >> k) & 1)
                result &= Parent::insert(e, 0, k);
            else
                result &= Parent::insert(e, 1, k);
        }
        return result;
    }

    void remove(uint64_t item, uint32_t class_id)
    {
        for (int k = 0; k < max_offset; ++k)
        {
            if ((class_id >> k) & 1)
                Parent::remove(item, 0, k);
            else
                Parent::remove(item, 1, k);
        }
        Parent::pos_map.erase(item);
        Parent::neg_map.erase(item);
        // cout << Parent::v_buckets[9].get_root_bucket() << " " << Parent::v_buckets[10].get_root_bucket() << endl;
    }

};

#endif //COLORINGCLASSIFER_SHIFT_COLORING_FILTER_H
