#include "dbms.h"

//./dbms --file file.data --query 'SPUSH mystack item'
int main(int argc, char** argv) {
    string file = "file.data", query;
    for (int i = 1; i + 1 < argc; i += 2) {
        string key = argv[i];
        if (key == "--file") file = argv[i + 1];
        else if (key == "--query") query = argv[i + 1];
    }

    Db db;
    loadDb(db, file);

    if (!query.empty()) {
        cout << execute(db, query) << endl;
        saveDb(db, file);
        clearDb(db);
        return 0;
    }

    cout << "Файл данных: " << file << ". Введите команду (exit - выход)\n";
    string line;
    while (cout << "> " && getline(cin, line)) {
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;
        cout << execute(db, line) << "\n";
        saveDb(db, file);// запись в файл после каждой команды
    }
    clearDb(db);
    return 0;
}
