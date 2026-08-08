#pragma once
#include "class.hpp"
#include "ast.hpp"
#include <memory>

namespace lox::runtime {
class Instance : public std::enable_shared_from_this<Instance> {
//class Instance {
private:
    std::shared_ptr<Class> cls;
    // インスタンスごとに異なるプロパティ（フィールド）を保持する辞書
    std::unordered_map<std::string, Value> fields;

public:
    explicit Instance(std::shared_ptr<Class> cls) 
        : cls(std::move(cls)) {}

    // プロパティの読み取り (GetExpr で呼ばれる)
    //Value get(const std::string& name) {
    Result<lox::Value> _get(const std::string& name) {
        // 1. まず自分のフィールド(インスタンス変数)を探す
        if (fields.contains(name)) {
            return fields.at(name);
        }

        // 2. なければ、所属するクラスのメソッドを探す
        if (cls->methods.contains(name)) {
            // ※ 将来的にここで this を束縛した「バウンドメソッド」を返します
            return cls->methods.at(name);
        }

        // どちらにもなければ実行時エラー
        //throw InterpreterError("Undefined property '" + name + "'.");
        return std::unexpected(InterpreterError("Undefined property '" + name + "'."));
    }
    Result<lox::Value> get(const std::string& name);

    // プロパティの書き込み (SetStmt で呼ばれる)
    void _set(const std::string& name, Value value) {
        // 既存の値を上書きするか、新しいキーとして追加する
        fields[name] = std::move(value);
    }
    void set(const std::string& name, Value value);
};

} // namespace lox::backend
