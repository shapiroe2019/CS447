#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <mutex>

using namespace std;

const int NUM_THREADS = 4;
mutex fileMutex;

class cityInfo {
private:
    wstring name;
    float min;
    float max;
    float sum;
    int total;

public:
    cityInfo() {
        min = 0;
        max = 0;
        sum = 0;
        total = 1;
    }
    cityInfo(wstring city, float temp) {
        name = city;
        min = temp;
        max = temp;
        sum = temp;
        total = 1;
    }

    void update(float temp) {
        if (temp < min) min = temp;
        if (temp > max) max = temp;
        sum += temp;
        total++;
    }

    void merge(cityInfo stats) {
        if (stats.min < min) min = stats.min;
        if (stats.max > max) max = stats.max;
        sum += stats.sum;
        total += stats.total;
    }

    void display() const{
        float mean = sum / total;
        float roundedMin = round(min * 10.0) / 10.0;
        float roundedMax = round(max * 10.0) / 10.0;
        float roundedMean = round(mean * 10.0) / 10.0;
        wcout << name << "=" << fixed << setprecision(1) << roundedMin << "/" << roundedMean << "/" << roundedMax;
    }
};

int main(int argc, char** argv)
{


    string input = argv[1];
    wifstream inputFile(input, ios::in | ios::binary);
    inputFile.imbue(locale(locale(), new codecvt_utf8<char>));

    map<wstring, cityInfo> cityStats;
    vector<thread> threads;
    vector<map<wstring, cityInfo>> map_buffers(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&inputFile, &map_buffers, i]() {
            while (true) {
                wstring line;
                unique_lock<mutex> lock(fileMutex);
                if (!getline(inputFile, line)) {
                    break;
                }
                lock.unlock();
                wistringstream stream(line);
                wstring token;
                vector<wstring> tokens;
                while (getline(stream, token, L';')) {
                    tokens.push_back(token);
                }
                wstring city = tokens[0];
                float temp = stof(tokens[1]);

                auto it = map_buffers[i].find(city);
                if (it != map_buffers[i].end()) {
                    map_buffers[i][city].update(temp);
                }
                else {
                    map_buffers[i][city] = cityInfo(city, temp);
                }
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    inputFile.close();

    for (int i = 0; i < NUM_THREADS; i++) {
        for (const auto& pair : map_buffers[i]) {
            wstring city = pair.first;
            cityInfo stats = pair.second;
            auto it = cityStats.find(city);
            if (it != cityStats.end()) {
                cityStats[city].merge(stats);
            }
            else {
                cityStats.emplace(city, stats);
            }
        }
    }

    for (const auto& pair : cityStats) {
        wstring city = pair.first;
        pair.second.display();
        cout << ", ";
    }
}