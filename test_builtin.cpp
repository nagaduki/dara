// test_interpreter.cpp
#include "doctest.h"
#include "interpreter.hpp"  // Source クラス等のために必要
#include "parser.hpp"
// #include "lexer.hpp"            // Source クラス等のために必要
// #include "combinator.hpp"
#include <memory>
#include <string>

#include "ast.hpp"  // ASTノードの定義が必要
#include "test_helper.hpp"
#include "value.hpp"
#include "builtin.hpp"

// #include <doctest/doctest.h>
// #include <iostream>
#include <memory>
// #include <string>
// #include <vnariant>

using namespace dara::frontend;
using namespace dara::backend;


TEST_CASE("Interpreter: 'is' and '===' Operators") {
    // 既存の素晴らしいヘルパー関数を利用します
    auto run_script_and_get = [](const char* input,
                                 const std::string& var_name) -> dara::Value {
        Source s(input);
        dara::frontend::Parser p(&s);
        dara::backend::Interpreter interpreter;
        dara::Program program;

        while (!s.isEnd()) {
            spaces(&s);
            if (s.isEnd()) break;
            auto res = p.decl();
            if (!res) throw std::runtime_error("Parse error: " + res.error().message);
            program.declarations.push_back(std::move(res.value()));
        }

        for (const auto& decl : program.declarations) {
            auto exec_res = interpreter.exec(decl.get());
            if (!exec_res) throw std::runtime_error(exec_res.error().message);
        }

        return interpreter.get_variable(var_name).value();
    };

    SUBCASE("1. 'is' operator checks inheritance properly") {
        const char* script = R"(
            class Animal {}
            class Dog extends Animal {}
            class Cat extends Animal {}
            
            let dog = Dog.new();
            
            let self_check = dog is Dog;
            let parent_check = dog is Animal;
            let sibling_check = dog is Cat;
        )";
        
        CHECK(std::get<bool>(run_script_and_get(script, "self_check").data) == true);
        CHECK(std::get<bool>(run_script_and_get(script, "parent_check").data) == true);
        CHECK(std::get<bool>(run_script_and_get(script, "sibling_check").data) == false);
    }

    SUBCASE("2. '===' operator checks EXACT class properly (Proper Type)") {
        const char* script = R"(
            class Animal {}
            class Dog extends Animal {}
            
            let dog = Dog.new();
            
            let self_exact = dog === Dog;
            let parent_exact = dog === Animal;
        )";
        
        // 自身のクラスとは一致する
        CHECK(std::get<bool>(run_script_and_get(script, "self_exact").data) == true);
        // 親クラスとは一致しない（ここが `is` との違い！）
        CHECK(std::get<bool>(run_script_and_get(script, "parent_exact").data) == false);
    }

    SUBCASE("3. '===' operator checks instance identity (Memory Address)") {
        const char* script = R"(
            class Box {}
            
            let b1 = Box.new();
            let b2 = Box.new();
            let b3 = b1;
            
            let self_id = b1 === b1;
            let diff_id = b1 === b2;
            let ref_id = b1 === b3;
        )";
        
        // 自分自身とは一致する
        CHECK(std::get<bool>(run_script_and_get(script, "self_id").data) == true);
        // newで別に作られたインスタンスとは一致しない
        CHECK(std::get<bool>(run_script_and_get(script, "diff_id").data) == false);
        // 同じ参照を持つ変数とは一致する
        CHECK(std::get<bool>(run_script_and_get(script, "ref_id").data) == true);
    }

    SUBCASE("4. Error handling and falsy behavior with primitive types") {
        const char* script = R"(
            class Box {}
            let b = Box.new();
            
            let check1 = b is 123;
            let check2 = b === "hello";
        )";

        // 動的言語として、型の不一致は false を返すのが自然
        CHECK(std::get<bool>(run_script_and_get(script, "check1").data) == false);
        CHECK(std::get<bool>(run_script_and_get(script, "check2").data) == false);
        
        // 逆に左辺が primitive の場合
        const char* script_primitive = R"(
            class Box {}
            let num = 123;
            let check = num is Box;
        )";
        CHECK(std::get<bool>(run_script_and_get(script_primitive, "check").data) == false);
    }
}


TEST_CASE("Interpreter: Builtin Functions") {
    // 既存の素晴らしいヘルパー関数をここでも利用します
    auto run_script_and_get = [](const char* input,
                                 const std::string& var_name) -> dara::Value {
        Source s(input);
        dara::frontend::Parser p(&s);
        dara::backend::Interpreter interpreter;
        dara::Program program;

        while (!s.isEnd()) {
            spaces(&s);
            if (s.isEnd()) break;
            auto res = p.decl();
            if (!res) throw std::runtime_error("Parse error: " + res.error().message);
            program.declarations.push_back(std::move(res.value()));
        }

        for (const auto& decl : program.declarations) {
            auto exec_res = interpreter.exec(decl.get());
            if (!exec_res) throw std::runtime_error(exec_res.error().message);
        }

        return interpreter.get_variable(var_name).value();
    };

    SUBCASE("len() function returns correct length") {
        // 配列の長さ
        dara::Value arr_len = run_script_and_get(
            "let a = [1, 2, 3, 4];"
            "let res = len(a);",
            "res");
        CHECK(std::get<int>(arr_len.data) == 4);

        // 文字列の長さ
        dara::Value str_len = run_script_and_get(
            "let s = \"hello\";"
            "let res = len(s);",
            "res");
        CHECK(std::get<int>(str_len.data) == 5);
    }

    SUBCASE("type() function returns correct type string") {
        CHECK(std::get<std::string>(run_script_and_get("let res = type(\"hi\");", "res").data) == "string");
        CHECK(std::get<std::string>(run_script_and_get("let res = type([1]);", "res").data) == "vector");
        CHECK(std::get<std::string>(run_script_and_get("let res = type(true);", "res").data) == "bool");
        
        // ⚠️ 注: あなたの BuiltinType の実装で "interger" と綴られていたため、テストもそれに合わせています。
        // もし "integer" に直した場合は、ここも "integer" に変更してください。
        CHECK(std::get<std::string>(run_script_and_get("let res = type(123);", "res").data) == "integer");
    }

    SUBCASE("assert() function stops execution on failure") {
        // 正常系 (Truthy): そのまま true が返る
        dara::Value pass_res = run_script_and_get("let res = assert(true, \"ok\");", "res");
        CHECK(std::get<bool>(pass_res.data) == true);

        // 異常系 (Falsy): 実行時エラー(std::runtime_error)が飛ぶはず
        bool caught_error = false;
        try {
            run_script_and_get("assert(false, \"This should fail\");", "dummy");
        } catch (const std::runtime_error& e) {
            caught_error = true;
            std::string err_msg = e.what();
            // エラーメッセージに指定した文字列が含まれているかチェック
            CHECK(err_msg.find("This should fail") != std::string::npos);
        }
        CHECK(caught_error == true);
    }

    SUBCASE("props() function returns fields and methods") {
        const char* script =
            "class Foo { method() {} }"
            "let f = Foo();"
            "f.x = 10;"
            "let res = props(f);";

        dara::Value props_val = run_script_and_get(script, "res");
        
        // 結果は std::vector<dara::Value> のはず
        auto* vec_ptr = std::get_if<std::vector<dara::Value>>(&props_val.data);
        REQUIRE(vec_ptr != nullptr);
        REQUIRE(vec_ptr->size() == 2);

        // unordered_map なので順序は保証されないため、中身を検索して両方あるか確認
        bool has_x = false;
        bool has_method = false;
        for (const auto& v : *vec_ptr) {
            std::string prop_name = std::get<std::string>(v.data);
            if (prop_name == "x") has_x = true;
            if (prop_name == "method()") has_method = true;
        }
        CHECK(has_x == true);
        CHECK(has_method == true);
    }
}
