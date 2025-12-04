#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip> // 用于格式化输出
#include "network_client.h"
#include "bike.pb.h"
#include "eventtype.h"

const std::string IP = "127.0.0.1";
const int PORT = 8080;
const int THREAD_NUM = 100;
const int TOTAL_REQUESTS = 1000;

std::atomic<int> g_success_count{0};
std::atomic<int> g_fail_count{0};

void worker_func(int id) {
  NetworkClient client(IP, PORT);
  if (!client.start()) {
    std::cerr << "Thread " << id << " connect failed." << std::endl;
    return;
  }

  tutorial::mobile_request req;
  req.set_mobile("1380000" + std::to_string(id));
  std::string body;
  req.SerializeToString(&body);

  for (int i = 0; i < TOTAL_REQUESTS; i++) {
    if (!client.sendRequest(EEVENTID_GET_MOBILE_CODE_REQ, body)) {
      g_fail_count++;
      continue;
    }

    int code;
    std::string msg;
    if (!client.getResponse(code, msg)) {
      g_fail_count ++;
      break;
    } else {
      if (code == ERRC_SUCCESS) g_success_count++;
      else g_fail_count ++;
    }
  }
}

int main() {
  std::cout << "=== Benchmark Start ===" << std::endl;
  std::cout << "Threads: " << THREAD_NUM << ", Requests/Thread: " << TOTAL_REQUESTS << std::endl;

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  for (int i = 0; i < THREAD_NUM; i++) {
    threads.emplace_back(worker_func, i);
  }

  //加个监控
  std::thread monitor([]() {
    long long last_count = 0;
    long long total_expected = (long long)THREAD_NUM * TOTAL_REQUESTS;
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒顺心一IC
      
      long long current = g_success_count + g_fail_count;
      long long speed = current - last_count; // 这一秒内处理了多少，就是实时 QPS
      last_count = current;

      double progress = (double) current / total_expected * 100.0;

      std::cout << "\r[Running]"
      << "[" << std::setw(3) << (int)progress << "%] "
      << "Done: " << current << "/" << total_expected
      << " | Speed: " << speed << "/s  " << std::flush;

      if (current >= total_expected) {
        break;
      }
    }
    std::cout << std::endl;
  });


  for (auto &t : threads) {
    t.join();
  }
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> diff = end - start;

  int total = g_success_count + g_fail_count;
  double qps = total / diff.count();

  std::cout << "\n=== Result ===" << std::endl;
  std::cout << "Time Elapsed: " << diff.count() << " s" << std::endl;
  std::cout << "Total Requests: " << total << std::endl;
  std::cout << "Success: " << g_success_count << std::endl;
  std::cout << "Failed: " << g_fail_count << std::endl;
  std::cout << "QPS: " << qps << " (req/sec)" << std::endl;
  return 0;
}