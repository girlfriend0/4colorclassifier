#include <iostream>
#include <ctime>
#include <random>
#include <algorithm>
#include "coloring_classifier.h"
#include "shift_coloring_classifier.h"
#include "coded_bloom_filter.h"
#include "multi_bloom_filter.h"
#include "shifting_bloom_filter.h"
#include "counting_bloom_filter.h"
#include "cache.h"

#define MAXN 1000
#define RATE 5
#define CLASSNUM 16

void test_two_set()
{
    // generate 10k data randomly
    vector<pair<uint64_t, uint32_t>> data(MAXN);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, 1ull << 40);

    unordered_set<uint64_t> filter;

    for (int i = 0; i < MAXN; ++i) {
        uint64_t item;
        while (1) {
            item = dis(gen);
            if (filter.find(item) == filter.end()) {
                filter.insert(item);
                break;
            }
        }
        data[i].first = item;
        data[i].second = i % CLASSNUM;
    }

    cout << data[0].first << endl;

    // auto cc = new ShiftingColoringClassifier<int(MAXN * 1.11 * log2(CLASSNUM)), 4, CLASSNUM>();
    // auto cc = new MultiBloomFilter<int(MAXN * 10), 4, 2>();
    auto cc = new CountingBloomFilterGroup<int(MAXN * 1.11 * log2(CLASSNUM)), 4, CLASSNUM>();

    cout << "Start building 4-color classifier for " << MAXN << " items" << endl;
    bool build_result = true;
    for (int i = 0; i < MAXN; i++)
    {
        cc->insert(data[i].first, data[i].second);
    }

    cout << "Finish building 4-color classifier for " << MAXN << " items" << endl;
    cout << "Using " << int(MAXN * 1.11) << " buckets" << endl;
    cout << "Build result: " << (build_result ? "success": "failed") << endl;

    if (build_result) {
        int err_cnt = 0;
        for (int i = 0; i < MAXN; ++i)
        {
            vector<uint32_t> result = cc->query(data[i].first);
            bool err = (find(result.begin(), result.end(), data[i].second) == result.end());
            err_cnt += (err);
            if (err)
                cout << i << endl;
        }
        cout << "Error count: " << err_cnt << endl;
    }

    for (int k = 0; k < 10; k++)
    {
        clock_t t1;
        t1 = clock();
        for (int i = 0; i < MAXN / RATE; i++)
        {
            cc->remove(data[i].first, data[i].second);
        }
        t1 = clock() - t1;
        cout << "Deletion finished" << endl;


        for (int i = 0; i < MAXN / RATE; ++i)
        {
            uint64_t item;
            while (1)
            {
                item = dis(gen);
                if (filter.find(item) == filter.end())
                {
                    filter.insert(item);
                    break;
                }
            }
            data[i].first = item;
            data[i].second = i % CLASSNUM;
        }

        cout << data[0].first << endl;

        cout << "Start building 4-color classifier for " << MAXN << " items" << endl;
        // bool build_result = cc->build(data, MAXN);

        t1 = clock() + t1;
        for (int i = 0; i < MAXN / RATE; i++)
        {
            cc->insert(data[i].first, data[i].second);
        }
        t1 = clock() - t1;
        // cout << t1 << endl;

        cout << "Finish building 4-color classifier for " << MAXN << " items" << endl;
        cout << "Build 4-color classifier for " << MAXN << " items" << endl;
        cout << "Using " << int(MAXN * 1.11) << " buckets" << endl;
        cout << "Build result: " << (build_result ? "success" : "failed") << endl;

        if (build_result)
        {
            int err_cnt = 0;
            for (int i = 0; i < MAXN; ++i)
            {
                vector<uint32_t> result = cc->query(data[i].first);
                bool err = (find(result.begin(), result.end(), data[i].second) == result.end());
                err_cnt += (err);
                if (err)
                    cout << i << endl;
            }
            cout << "Error count: " << err_cnt << endl;
        }
    }
}

void cbf_test()
{
    auto cbf = CountingBloomFilter<8, 2, 2, 2>();
    int a = 0, b = 0, c = 0;
    while(a >= 0)
    {
        cin >> a >> b;
        switch (a)
        {
            case 1:
                cin >> c;
                cbf.insert(b, c);
                break;
            case 2:
                cbf.remove(b);
                break;
            case 3:
                cin >> c;
                cout << bool(cbf.query_bf(b, c)) << endl;
                break;
            case 4:
                cout << cbf.query_multiway(b).size() << endl;
                break;
            case 5:
                cbf.update(1, cbf.flip);
                break;
        }
        cout << cbf.bf[0] << " " << cbf.count[0] << endl;
    }
}

void cache_test()
{
    Cache cache;
    cache.set_size(3);
    uint64_t key;
    int len = 0;
    cout << "test" << endl;
    while (len >= 0)
    {
        cin >> key >> len;
        pair<string, int> res = cache.get(key);
        cout << bool(res.second >= 0) << endl;
        if (res.second < 0)
        {
            cache.set(key, "", len);
            cache.print();
        }
    }
}

void cache_cbf_test()
{
    //CountingBloomFilterCache<64, 4, 2, 4> cache(3, 1);
    ColoringClassifierCache<64, 4, 2> cache(3, 1);
    uint64_t key;
    uint32_t idx;
    int len = 0;
    cout << "test" << endl;
    while (len >= 0)
    {
        cin >> key >> len >> idx;
        bool res = cache.get(key, "", len, idx);
        cout << res << endl;
        if (!res)
        {
            int t = 0;
            while (t >= 0)
            {
                cin >> t;
                cout << cache.cache[0].get(t).second << " " << cache.cache[1].get(t).second << endl;
                for (auto i: cache.cc.query_multiway(t))
                    cout << i << " ";
                cout << endl;
                for (auto i: cache.cc.query_multiway(t))
                    cout << i << " ";
                cout << endl;
            }
        }
    }
}

int main()
{
    cache_cbf_test();
    return 0;
}