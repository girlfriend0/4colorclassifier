#ifndef COLORINGCLASSIFER_CACHE_H
#define COLORINGCLASSIFER_CACHE_H

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "shift_coloring_classifier.h"
#include "counting_bloom_filter.h"

using namespace std;

struct CacheNode {
    uint64_t key;
    string value;
    int size;
    CacheNode *pre, *next;
    CacheNode(int k, string s, int l) : key(k), value(s), size(l), pre(NULL), next(NULL) {}
};

class LRUCache
{
protected:
    int size;
    int recent;
    CacheNode *head, *tail;
    map<uint64_t, CacheNode *> mp;

public:
    LRUCache(int capacity=0)
    {
        size = capacity;
        recent = 0;
        head = NULL;
        tail = NULL;
    }

    pair<string, int> get(uint64_t key)
    {
        auto it = mp.find(key);
        if (it != mp.end())
        {
            CacheNode *node = it -> second;
            remove(node);
            setHead(node);
            // return node -> value;
            return make_pair(node->value, node->size);
        }
        else
        {
            return make_pair("", -1);
        }
    }

    vector<uint64_t> set(uint64_t key, string value, int len)
    {
        vector<uint64_t> removed;
        auto it = mp.find(key);
        if (it != mp.end())
        {
            CacheNode *node = it -> second;
            node -> value = value;
            remove(node);
            setHead(node);
        }
        else
        {
            CacheNode *newNode = new CacheNode(key, value, len);
            while (recent + len > size and !mp.empty())
            {
                auto iter = mp.find(tail -> key);
                recent -= tail->size;
                removed.push_back(tail->key);
                remove(tail);
                mp.erase(iter);

            }
            setHead(newNode);
            mp[key] = newNode;
            recent += newNode->size;
        }
        return removed;
    }

    void remove(CacheNode *node)
    {
        if (node -> pre != NULL)
        {
            node -> pre -> next = node -> next;
        }
        else
        {
            head = node -> next;
        }
        if (node -> next != NULL)
        {
            node -> next -> pre = node -> pre;
        }
        else
        {
            tail = node -> pre;
        }
    }

    void setHead(CacheNode *node)
    {
        node -> next = head;
        node -> pre = NULL;

        if (head != NULL)
        {
            head -> pre = node;
        }
        head = node;
        if (tail == NULL)
        {
            tail = head;
        }
    }

    int num()
    {
        return mp.size();
    }

    void set_size(int capacity)
    {
        size = capacity;
    }
};


class Cache: public LRUCache
{

public:
    vector<uint64_t> inserted, removed, new_removed;

    bool set(uint64_t key, string value, int len)
    {
        inserted.push_back(key);
        new_removed = LRUCache::set(key, value, len);
        for (auto r: new_removed)
            removed.push_back(r);
        return true;
    }

    void print()
    {
        cout << "inserted: ";
        for (auto k: inserted)
            cout << k << ", ";
        cout << endl;
        cout << "removed: ";
        for (auto k: removed)
            cout << k << ", ";
        cout << endl;
        cout << "new removed: ";
        for (auto k: new_removed)
            cout << k << ", ";
        cout << endl;
    }

};

template<int num_bits, int k, int class_num = 2, int count_bits=4>
class CountingBloomFilterCache
{
public:
    double threshold = 0.01;
    Cache cache[class_num];
    CountingBloomFilter<num_bits, k, class_num, count_bits> cbf[class_num];

public:
    CountingBloomFilterCache(int capacity, double thres=0.01)
    {
        threshold = thres;
        for (int i = 0; i < class_num; i++)
        {
            cbf[i].class_idx = i;
            cache[i].set_size(capacity);
        }
    }

    int get(uint64_t key, string value, int len, uint32_t idx)
    {
        pair<string, int> res = cache[idx].get(key);
        if (res.second < 0)
        {
            vector<uint32_t> other = cbf[idx].query_multiway(key); // must syn local summary!!
            for (auto i: other)
                if (i != idx)
                {
                    res = cache[i].get(key);
                    if (res.second >= 0)
                    {
                        value = res.first;
                        len = res.second;
                        break;
                    }
                }
            cache[idx].set(key, value, len);
            cbf[idx].insert(key);
            for (auto r: cache[idx].new_removed)
                cbf[idx].remove(r);
            check_update(idx);
            return 0;
        }
        else
            return 1;
    }

    void check_update(uint32_t idx)
    {
        if (cache[idx].inserted.size() >= threshold * cache[idx].num() or
        cache[idx].removed.size() >= threshold * cache[idx].num())
            update(idx);
    }

    void update(uint32_t idx)
    {
        cout << "update " << idx << endl;
        for (uint32_t i = 0; i < class_num; i++)
            if (i != idx)
                cbf[i].update(idx, cbf[idx].flip);
        cbf[idx].flip.first.clear();
        cbf[idx].flip.second.clear();
        cache[idx].inserted.clear();
        cache[idx].removed.clear();
    }

};

template<uint32_t bucket_num, uint32_t color_num, uint32_t class_num>
class ColoringClassifierCache
{
public:
    double threshold = 0.01;
    Cache cache[class_num];
    ShiftingColoringClassifier<bucket_num, color_num, class_num> cc;

public:
    ColoringClassifierCache(int capacity, double thres=0.01)
    {
        threshold = thres;
        for (int i = 0; i < class_num; i++)
        {
            cache[i].set_size(capacity);
        }
    }

    int get(uint64_t key, string value, int len, uint32_t idx)
    {
        pair<string, int> res = cache[idx].get(key);
        if (res.second < 0)
        {
            vector<uint32_t> other = cc.query(key); // must syn local summary!!
            for (auto i: other)
                if (i != idx)
                {
                    res = cache[i].get(key);
                    if (res.second >= 0)
                    {
                        value = res.first;
                        len = res.second;
                        break;
                    }
                }
            cache[idx].set(key, value, len);
            check_update(idx);
            return 0;
        }
        else
            return 1;
    }

    void check_update(uint32_t idx)
    {
        if (cache[idx].inserted.size() >= threshold * cache[idx].num() or
            cache[idx].removed.size() >= threshold * cache[idx].num())
            update(idx);
    }

    void update(uint32_t idx)
    {
        cout << "update " << idx << endl;
        for (auto key: cache[idx].removed)
            cc.remove(key, idx);
        for (auto key: cache[idx].inserted)
            cc.insert(key, idx);
        cache[idx].inserted.clear();
        cache[idx].removed.clear();
    }
};
#endif //COLORINGCLASSIFER_CACHE_H
