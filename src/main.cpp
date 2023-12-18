#include <fmt/format.h>
#include <memory>
#include <vector>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <asynccurl/executor.h>
#include <asynccurl/request.h>
#include <asynccurl/spwan.h>
#include <coroutine>
#include <main/main.h>

#include <asynccurl/awaitable_request.h>
#include <asynccurl/task.h>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>
#include <string>

#include <nlohmann/json.hpp>
// template <typename T> struct Awaitable_task {
// public:
//   Awaitable_task(Task<T> &request, asynccurl::Executor &executor)
//       : slot_(request), executor_(&executor) {}
//   bool await_ready() const noexcept { return false; }
//   void await_suspend(std::coroutine_handle<> handle) {
//     slot_.set_on_finished([handle] { handle(); });
//     slot_.resume();
//   }

//   T await_resume() const noexcept { return slot_.get_result(); }

//   Task<T> &slot_;
//   asynccurl::Executor *executor_;
// };

struct Get_resume_handle {
public:
  bool await_ready() const noexcept { return false; }
  bool await_suspend(std::coroutine_handle<> handle) {
    hd = handle;
    return false;
  }

  std::coroutine_handle<> await_resume() const noexcept { return hd; }

  std::coroutine_handle<> hd;
};

Task<std::string> fetch(std::string url, asynccurl::Executor &executor) {
  auto buffer = std::make_shared<asynccurl::Write_string>();
  asynccurl::Request_slot req{url};
  req.set_write_buffer(buffer);

  co_await asynccurl::Awaitable_request{req, executor};

  SPDLOG_DEBUG("result: {}", buffer->buffer_);

  // SPDLOG_INFO("{}", nlohmann::json::parse(buffer->buffer_).dump());

  co_return buffer->buffer_;
}
Task<nlohmann::json> fetch_json(std::string url,
                                asynccurl::Executor &executor) {

  auto sub_task = fetch(url, executor);
  //  asynccurl::spawn(executor, sub_task);

  // auto str = co_await Awaitable_task<std::string>{sub_task, executor,
  //  "task fetch json " + url};
  std::string str;

  str = co_await std::move(sub_task);

  // co_await std::suspend_always{};
  nlohmann::json json;
  try {
    json = nlohmann::json::parse(str);
  } catch (...) {
  }
  co_return json;
}
Task<std::vector<std::string>> fetch_image_tags(std::string url,
                                                asynccurl::Executor &executor) {
  auto json = co_await fetch_json(url, executor);
  std::vector<std::string> tags;
  for (auto tag : json["tags"]) {
    tags.push_back(tag);
  }
  co_return tags;
}

int main(int argc, char **argv) {
  asynccurl::Executor executor;
  spdlog::set_level(spdlog::level::off);
  std::string domain = "xxx";
  std::string base_url = fmt::format("https://{}:5000", domain);
  std::string repos_url = fmt::format("{}/v2/_catalog", base_url);

  enum actions { list, search };
  actions action = list;
  std::string search_name;
  if (argc > 2 && argv[1] == std::string("search")) {
    search_name = argv[2];
    action = search;
  }

  // spawn a task
  auto task = fetch_json(repos_url, executor);
  auto fetch_task = [](auto &exe, auto &t,
                                          std::string base_url,actions action, std::string search_name) -> Task<char> {
    auto result = co_await std::move(t);
    SPDLOG_INFO("response json: {}", result.dump(2));

    for (auto repo : result["repositories"]) {
      auto link =
          fmt::format("{}/v2/{}/tags/list", base_url, std::string(repo));
      if (action == list) {
        std::cout << std::string(repo) << "\n";
      }else if (action == search) {
        if (std::string(repo).find(search_name) != std::string::npos) {
            std::cout << fmt::format("{}\n", std::string(repo));
        }
      }
      // break;
      auto tags = co_await fetch_image_tags(link, exe);
      for (auto t : tags) {
        SPDLOG_INFO("repo: {} tag: {}", std::string(repo), t);
        if (action == search) {
          if (std::string(repo).find(search_name) != std::string::npos) {
            std::cout << fmt::format("\t{}\n", t);
          }
        }
      }
    }
    co_return '0';
  }(executor, task, base_url,action,search_name);
  asynccurl::spawn(executor, fetch_task);

  executor.run();
  return 0;
}
