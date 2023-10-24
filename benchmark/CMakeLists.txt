find_package(benchmark CONFIG REQUIRED)

function(add_bench target file)

add_executable(${target} ${file})
target_link_libraries(${target} PRIVATE benchmark::benchmark benchmark::benchmark_main)

endfunction()

add_bench(str str.cpp)