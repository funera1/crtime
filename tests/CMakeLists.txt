add_subdirectory(third_party/googletest)

# テスト用実行ファイルの作成
add_executable(hello_test hello_test.cpp)

# テスト対象のライブラリをリンク
target_link_libraries(hello_test PRIVATE gtest gtest_main)

# CTest に登録
add_test(NAME HelloTest COMMAND hello_test)