SUBCASE("props() function returns fields and methods") {
        const char* script =
            "class Foo { method() {} }"
            "let f = Foo();"
            "f.x = 10;"
            "let res = props(f);";

        Value props_val = run_script_and_get(script, "res");

        // 結果は std::vector<Value> のはず
        auto* vec_ptr = std::get_if<Array>(&props_val.data);
        REQUIRE(vec_ptr != nullptr);
        REQUIRE(vec_ptr->size() == 2);

        // unordered_map なので順序は保証されないため、中身を検索して両方あるか確認
        bool has_x = false;
        bool has_method = false;
        for (const auto& v : *vec_ptr) {
            // 🌟 修正ポイント1：要素は Array ではなく String (文字列) として取り出す
            const String* prop_name = std::get_if<String>(&v.data);
            
            if (prop_name != nullptr) {
                // 🌟 修正ポイント2：文字列同士なので正常に == で比較できる！
                if (*prop_name == "x") {
                    has_x = true;
                }
                // ※注意：クラスの実装によっては "method" という名前で登録されている可能性があります。
                // テストが落ちる場合は "method" に直してみてください。
                if (*prop_name == "method()" || *prop_name == "method") { 
                    has_method = true;
                }
            }
        }
        CHECK(has_x == true);
        CHECK(has_method == true);
    }
