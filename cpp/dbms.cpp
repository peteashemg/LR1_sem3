#include "dbms.h"

// коды команд
enum Op {
    OP_APUSH = 1, OP_ADEL, OP_AGET, OP_ASET, OP_ALEN,
    OP_LPUSH, OP_LPUSHH, OP_LPUSHB, OP_LPUSHA,
    OP_LDEL, OP_LDELH, OP_LDELT, OP_LDELB, OP_LDELA,
    OP_LFIND, OP_LGET, OP_LGETR, OP_LGETI,
    OP_SPUSH, OP_SPOP, OP_SPEEK,
    OP_QPUSH, OP_QPOP, OP_QPEEK,
    OP_TINSERT, OP_TADD, OP_TSEARCH, OP_TCOMPLETE, OP_TGET, OP_TPRE, OP_TIN, OP_TPOST,
    OP_PRINT
};

static const string ARGS = "ERR: not enough arguments", NF = "ERR: not found",
                    EMPTY = "ERR: empty", RANGE = "ERR: index out of range";

static map<string, int> buildTable() {
    map<string, int> t = {
        {"MPUSH", OP_APUSH}, {"MDEL", OP_ADEL}, {"MGET", OP_AGET}, {"MSET", OP_ASET}, {"MLEN", OP_ALEN},
        {"SPUSH", OP_SPUSH}, {"SPOP", OP_SPOP}, {"SPEEK", OP_SPEEK},
        {"QPUSH", OP_QPUSH}, {"QPOP", OP_QPOP}, {"QPEEK", OP_QPEEK},
        {"TINSERT", OP_TINSERT}, {"TADD", OP_TADD}, {"TSEARCH", OP_TSEARCH}, {"ISMEMBER", OP_TSEARCH},
        {"TCOMPLETE", OP_TCOMPLETE}, {"TGET", OP_TGET}, {"PRINT", OP_PRINT}, {"LGETR", OP_LGETR},
        {"TPREORDER", OP_TPRE}, {"TINORDER", OP_TIN}, {"TPOSTORDER", OP_TPOST}};

    const map<string, int> lst = {
        {"PUSH", OP_LPUSH}, {"PUSHH", OP_LPUSHH}, {"PUSHB", OP_LPUSHB}, {"PUSHA", OP_LPUSHA},
        {"DEL", OP_LDEL}, {"DELH", OP_LDELH}, {"DELT", OP_LDELT}, {"DELB", OP_LDELB}, {"DELA", OP_LDELA},
        {"FIND", OP_LFIND}, {"GET", OP_LGET}, {"GETI", OP_LGETI}};
    for (const auto& [suffix, op] : lst)
        for (const char* pre : {"F", "L"}) t[string(pre) + suffix] = op;
    return t;
}

static int kindOf(char letter) {
    switch (letter) {
        case 'M': return ARR;   case 'F': return FLIST; case 'L': return LLIST;
        case 'S': return STACK; case 'Q': return QUEUE; case 'T': return TREE;
    }
    return 0;
}

static bool creates(int op) {
    switch (op) {
        case OP_APUSH: case OP_LPUSH: case OP_LPUSHH:
        case OP_SPUSH: case OP_QPUSH: case OP_TINSERT: return true;
    }
    return false;
}

//  списки / стек / очередь 
static bool dbl(const Container& c) { return c.kind == LLIST; }

static Node* findNode(const Container& c, const string& v) {
    for (Node* n = c.head; n; n = n->next) if (n->val == v) return n;
    return nullptr;
}

// 
static Node* pred(const Container& c, Node* n) {
    if (dbl(c)) return n->prev;
    Node* p = nullptr;
    for (Node* x = c.head; x && x != n; x = x->next) p = x;
    return p;
}

// вставка после p; p == nullptr вставка в голову
static void insertAfter(Container& c, Node* p, const string& v) {
    Node* n = new Node{v};
    if (!p) {
        n->next = c.head;
        if (dbl(c) && c.head) c.head->prev = n;
        c.head = n;
        if (!c.tail) c.tail = n;
    } else {
        n->next = p->next;
        if (dbl(c)) { n->prev = p; if (p->next) p->next->prev = n; }
        p->next = n;
        if (c.tail == p) c.tail = n;
    }
    c.size++;
}

static void eraseNode(Container& c, Node* n) {
    Node* p = pred(c, n);
    if (p) p->next = n->next; else c.head = n->next;
    if (n->next) { if (dbl(c)) n->next->prev = p; }
    else c.tail = p;
    delete n;
    c.size--;
}

//Complete Binary Tree вставка в первое свободное место при обходе в ширину
static void insertT(Container& c, const string& v) {
    TNode* n = new TNode{v};
    if (!c.root) c.root = n;
    else {
        queue<TNode*> q;
        q.push(c.root);
        while (!q.empty()) {
            TNode* x = q.front(); q.pop();
            if (!x->left)  { x->left = n;  break; }
            q.push(x->left);
            if (!x->right) { x->right = n; break; }
            q.push(x->right);
        }
    }
    c.size++;
}

static TNode* findT(TNode* r, const string& v) {
    if (!r) return nullptr;
    if (r->val == v) return r;
    TNode* x = findT(r->left, v);
    return x ? x : findT(r->right, v);
}

// обход в ширину после первого nullptr не должно быть узлов
bool isComplete(TNode* root) {
    if (!root) return true;
    queue<TNode*> q;
    q.push(root);
    bool gap = false;
    while (!q.empty()) {
        TNode* x = q.front(); q.pop();
        if (!x) { gap = true; continue; }
        if (gap) return false;
        q.push(x->left);
        q.push(x->right);
    }
    return true;
}

static vector<string> levelOrder(TNode* root) {
    vector<string> out;
    if (!root) return out;
    queue<TNode*> q;
    q.push(root);
    while (!q.empty()) {
        TNode* x = q.front(); q.pop();
        out.push_back(x->val);
        if (x->left)  q.push(x->left);
        if (x->right) q.push(x->right);
    }
    return out;
}

// обходы в глубину
static void walk(TNode* n, int mode, vector<string>& o) {
    if (!n) return;
    if (mode == 0) o.push_back(n->val);
    walk(n->left, mode, o);
    if (mode == 1) o.push_back(n->val);
    walk(n->right, mode, o);
    if (mode == 2) o.push_back(n->val);
}

static string join(const vector<string>& v) {
    string s;
    for (size_t i = 0; i < v.size(); i++) s += (i ? " " : "") + v[i];
    return s.empty() ? "(empty)" : s;
}

static void freeT(TNode* n) { if (n) { freeT(n->left); freeT(n->right); delete n; } }

static void printT(TNode* n, int d, string& out) {   // повёрнутое дерево: правое сверху
    if (!n) return;
    printT(n->right, d + 1, out);
    out += string(d * 4, ' ') + n->val + "\n";
    printT(n->left, d + 1, out);
}

//вывод
static string show(const Container& c) {
    string s;
    switch (c.kind) {
        case ARR:
            s = "[";
            for (size_t i = 0; i < c.arr.size(); i++) s += (i ? ", " : "") + c.arr[i];
            return s + "]";
        case FLIST: case LLIST:
            for (Node* n = c.head; n; n = n->next)
                s += (n == c.head ? "" : (c.kind == FLIST ? " -> " : " <-> ")) + n->val;
            return s.empty() ? "(empty)" : s;
        case STACK: case QUEUE:
            s = c.kind == STACK ? "top: " : "front: ";
            for (Node* n = c.head; n; n = n->next) s += n->val + " ";
            return c.head ? s : "(empty)";
        case TREE:
            printT(c.root, 0, s);
            if (s.empty()) return "(empty)";
            s.pop_back();
            return s;
    }
    return "";
}


static void ser(TNode* n, vector<string>& o) {
    if (!n) { o.push_back("#"); return; }
    o.push_back(n->val);
    ser(n->left, o);
    ser(n->right, o);
}

static TNode* deser(const vector<string>& t, size_t& i, int& cnt) {
    if (i >= t.size() || t[i] == "#") { i++; return nullptr; }
    TNode* n = new TNode{t[i++]};
    cnt++;
    n->left = deser(t, i, cnt);
    n->right = deser(t, i, cnt);
    return n;
}

static vector<string> toTokens(const Container& c) {
    vector<string> o;
    switch (c.kind) {
        case ARR:  return c.arr;
        case TREE: ser(c.root, o); return o;
        default:   for (Node* n = c.head; n; n = n->next) o.push_back(n->val);
    }
    return o;
}

void saveDb(const Db& db, const string& file) {
    ofstream out(file);
    for (const auto& [name, c] : db) {
        out << "MFLSQT"[c.kind - 1] << " " << name;
        for (const auto& tok : toTokens(c)) out << " " << tok;
        out << "\n";
    }
}

bool loadDb(Db& db, const string& file) {
    ifstream in(file);
    if (!in) return false;
    string line;
    while (getline(in, line)) {
        istringstream ls(line);
        string k, name, tok;
        if (!(ls >> k >> name)) continue;
        vector<string> tokens;
        while (ls >> tok) tokens.push_back(tok);
        Container& c = db[name];
        c.kind = kindOf(k[0]);
        switch (c.kind) {
            case ARR:  c.arr = tokens; break;
            case TREE: { size_t i = 0; c.root = deser(tokens, i, c.size); break; }
            default:   for (const auto& v : tokens) insertAfter(c, c.tail, v);
        }
    }
    return true;
}

void clearDb(Db& db) {
    for (auto& [name, c] : db) {
        for (Node* n = c.head; n;) { Node* nx = n->next; delete n; n = nx; }
        freeT(c.root);
    }
    db.clear();
}

static bool toIdx(const string& s, int lim, int& i) {
    try { i = stoi(s); } catch (...) { return false; }
    return i >= 0 && i < lim;
}

// выполнение запроса
string execute(Db& db, const string& q) {
    static const map<string, int> table = buildTable();
    istringstream in(q);
    vector<string> t;
    string w;
    while (in >> w) t.push_back(w);
    if (t.size() < 2) return "ERR: usage: COMMAND name [args]";

    auto it = table.find(t[0]);
    if (it == table.end()) return "ERR: unknown command " + t[0];
    int op = it->second;
    const string& name = t[1];
    vector<string> a(t.begin() + 2, t.end());
    auto need = [&](size_t n) { return a.size() >= n; };

    Container* c;
    auto f = db.find(name);
    if (f == db.end()) {
        if (!creates(op)) return "ERR: no such container: " + name;
        c = &db[name];
        c->kind = kindOf(t[0][0]);
    } else c = &f->second;

    bool generic = t[0] == "PRINT" || t[0] == "ISMEMBER";
    if (!generic && c->kind != kindOf(t[0][0])) return "ERR: wrong container type";
    if (t[0] == "ISMEMBER" && c->kind != TREE) return "ERR: not a tree";

    int i = 0;
    switch (op) {
        // массив
        case OP_APUSH: {
            if (!need(1)) return ARGS;
            i = c->arr.size();
            if (a.size() > 1 && !toIdx(a[1], c->arr.size() + 1, i)) return RANGE;
            c->arr.insert(c->arr.begin() + i, a[0]);
            return a[0];
        }
        case OP_ADEL: {
            if (!need(1)) return ARGS;
            if (!toIdx(a[0], c->arr.size(), i)) return RANGE;
            string v = c->arr[i];
            c->arr.erase(c->arr.begin() + i);
            return v;
        }
        case OP_AGET:
            if (!need(1)) return ARGS;
            if (!toIdx(a[0], c->arr.size(), i)) return RANGE;
            return c->arr[i];
        case OP_ASET:
            if (!need(2)) return ARGS;
            if (!toIdx(a[0], c->arr.size(), i)) return RANGE;
            c->arr[i] = a[1];
            return a[1];
        case OP_ALEN: return to_string(c->arr.size());

        //односвязный / двусвязный список
        case OP_LPUSH:  if (!need(1)) return ARGS; insertAfter(*c, c->tail, a[0]); return a[0];
        case OP_LPUSHH: if (!need(1)) return ARGS; insertAfter(*c, nullptr, a[0]); return a[0];
        case OP_LPUSHB: {
            if (!need(2)) return ARGS;
            Node* n = findNode(*c, a[0]);
            if (!n) return NF;
            insertAfter(*c, pred(*c, n), a[1]);
            return a[1];
        }
        case OP_LPUSHA: {
            if (!need(2)) return ARGS;
            Node* n = findNode(*c, a[0]);
            if (!n) return NF;
            insertAfter(*c, n, a[1]);
            return a[1];
        }
        case OP_LDEL: {
            if (!need(1)) return ARGS;
            Node* n = findNode(*c, a[0]);
            if (!n) return NF;
            eraseNode(*c, n);
            return "OK";
        }
        case OP_LDELH: if (!c->head) return EMPTY; eraseNode(*c, c->head); return "OK";
        case OP_LDELT: if (!c->tail) return EMPTY; eraseNode(*c, c->tail); return "OK";
        case OP_LDELB: {
            if (!need(1)) return ARGS;
            Node* n = findNode(*c, a[0]);
            if (!n) return NF;
            Node* p = pred(*c, n);
            if (!p) return "ERR: no element before";
            eraseNode(*c, p);
            return "OK";
        }
        case OP_LDELA: {
            if (!need(1)) return ARGS;
            Node* n = findNode(*c, a[0]);
            if (!n) return NF;
            if (!n->next) return "ERR: no element after";
            eraseNode(*c, n->next);
            return "OK";
        }
        case OP_LFIND:
            if (!need(1)) return ARGS;
            for (Node* n = c->head; n; n = n->next, i++)
                if (n->val == a[0]) return "TRUE (index " + to_string(i) + ")";
            return "FALSE";
        case OP_LGET: return show(*c);
        case OP_LGETI: {                                  // элемент по индексу
            if (!need(1)) return ARGS;
            if (!toIdx(a[0], c->size, i)) return RANGE;
            Node* n = c->head;
            while (i-- > 0) n = n->next;
            return n->val;
        }
        case OP_LGETR: {
            string s;
            for (Node* n = c->tail; n; n = n->prev) s += (n == c->tail ? "" : " <-> ") + n->val;
            return s.empty() ? "(empty)" : s;
        }

        // стек
        case OP_SPUSH: if (!need(1)) return ARGS; insertAfter(*c, nullptr, a[0]); return a[0];
        case OP_SPOP: case OP_QPOP: {
            if (!c->head) return EMPTY;
            string v = c->head->val;
            eraseNode(*c, c->head);
            return v;
        }
        case OP_SPEEK: case OP_QPEEK: return c->head ? c->head->val : EMPTY;

        //очередь 
        case OP_QPUSH: if (!need(1)) return ARGS; insertAfter(*c, c->tail, a[0]); return a[0];

        //Complete Binary Tree
        case OP_TINSERT:
            if (!need(1)) return ARGS;
            if (a[0] == "#") return "ERR: '#' is reserved";
            insertT(*c, a[0]);
            return a[0];
        case OP_TADD: {
            if (!need(3)) return ARGS;
            if (a[2] == "#" || (a[1] != "L" && a[1] != "R")) return "ERR: bad arguments";
            TNode* p = findT(c->root, a[0]);
            if (!p) return NF;
            TNode*& slot = a[1] == "L" ? p->left : p->right;
            if (slot) return "ERR: slot occupied";
            slot = new TNode{a[2]};
            c->size++;
            return a[2];
        }
        case OP_TSEARCH:
            if (!need(1)) return ARGS;
            return findT(c->root, a[0]) ? "TRUE" : "FALSE";
        case OP_TCOMPLETE: return isComplete(c->root) ? "TRUE" : "FALSE";
        case OP_TGET: return join(levelOrder(c->root));   // в ширину
        case OP_TPRE: case OP_TIN: case OP_TPOST: {
            int mode = 0;
            switch (op) { case OP_TIN: mode = 1; break; case OP_TPOST: mode = 2; break; }
            vector<string> o;
            walk(c->root, mode, o);
            return join(o);
        }

        case OP_PRINT: return show(*c);
    }
    return "ERR: unsupported";
}
