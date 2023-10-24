#include <fmt/format.h>
#include <memory>
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
//   Awaitable_task(Task<T> &request, asynccurl::Executor &executor,
//                  std::string name = "")
//       : slot_(request), executor_(&executor), name_{} {}
//   bool await_ready() const noexcept { return false; }
//   void await_suspend(std::coroutine_handle<> handle) {
//     SPDLOG_DEBUG("{} get suspend.", name_);
//     slot_.set_on_finished([handle] { handle(); });
//     slot_.resume();
//   }

//   T await_resume() const noexcept {
//     SPDLOG_DEBUG("{} get resumed.", name_);
//     return slot_.get_result();
//   }

//   Task<T> &slot_;
//   asynccurl::Executor *executor_;
//   std::string name_;
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
  auto buffer = std::make_shared<asynccurl::Write_string>();
  asynccurl::Request_slot req{url};
  req.set_write_buffer(buffer);

  co_await asynccurl::Awaitable_request{req, executor};

  SPDLOG_DEBUG("result: {}", buffer->buffer_);

  co_return nlohmann::json::parse(buffer->buffer_);

  auto sub_task = fetch(url, executor);
  //  asynccurl::spawn(executor, sub_task);

  // auto str = co_await Awaitable_task<std::string>{sub_task, executor,
  //  "task fetch json " + url};
  std::string str;

  auto self_handle = co_await Get_resume_handle{};

  sub_task.set_on_finished([&sub_task, &str, self_handle] {
    str = sub_task.get_result();
    self_handle();
  });

  asynccurl::spawn(executor, sub_task);
  co_await std::suspend_always{};
  nlohmann::json json;
  try {
    json = nlohmann::json::parse(str);
  } catch (...) {
  }
  co_return json;
}
int main() {
  asynccurl::Executor executor;
  // spdlog::set_level(spdlog::level::debug);
  std::string domain = "xxx";
  std::string base_url = fmt::format("https://{}:5000",domain);
  std::string repos_url = fmt::format("{}/v2/_catalog", base_url);
  // spawn a task
  auto task = fetch_json(repos_url, executor);
  task.set_on_finished([&task, &executor, base_url]() {
    auto result = task.get_result();
    std::cout << result.dump(2) << '\n';
    for (auto repo : result["repositories"]) {
      auto link =
          fmt::format("{}/v2/{}/tags/list", base_url, repo.get<std::string>());
      // break;
      auto t =
          std::make_shared<Task<nlohmann::json>>(fetch_json(link, executor));
      t->set_on_finished([link, t] {
        SPDLOG_INFO("{} done: {}", link, t->get_result().dump());
      });
      asynccurl::spawn(executor, *t);
    }
  });
  asynccurl::spawn(executor, task);
  executor.run();
  return 0;
}
