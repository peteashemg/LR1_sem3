package main

import (
	"bufio"
	"os"
	"strconv"
	"strings"
)

const (
	errArgs  = "ERR: not enough arguments"
	errNF    = "ERR: not found"
	errEmpty = "ERR: empty"
	errRange = "ERR: index out of range"
)

var table = buildTable()

func buildTable() map[string]int {
	t := map[string]int{
		"MPUSH": OpAPush, "MDEL": OpADel, "MGET": OpAGet, "MSET": OpASet, "MLEN": OpALen,
		"SPUSH": OpSPush, "SPOP": OpSPop, "SPEEK": OpSPeek,
		"QPUSH": OpQPush, "QPOP": OpQPop, "QPEEK": OpQPeek,
		"TINSERT": OpTInsert, "TADD": OpTAdd, "TSEARCH": OpTSearch, "ISMEMBER": OpTSearch,
		"TCOMPLETE": OpTComplete, "TGET": OpTGet, "PRINT": OpPrint, "LGETR": OpLGetR,
		"TPREORDER": OpTPre, "TINORDER": OpTIn, "TPOSTORDER": OpTPost,
	}
	lst := map[string]int{
		"PUSH": OpLPush, "PUSHH": OpLPushH, "PUSHB": OpLPushB, "PUSHA": OpLPushA,
		"DEL": OpLDel, "DELH": OpLDelH, "DELT": OpLDelT, "DELB": OpLDelB, "DELA": OpLDelA,
		"FIND": OpLFind, "GET": OpLGet, "GETI": OpLGetI,
	}
	for suffix, op := range lst {
		for _, pre := range []string{"F", "L"} {
			t[pre+suffix] = op
		}
	}
	return t
}

func kindOf(l byte) int {
	switch l {
	case 'M':
		return ARR
	case 'F':
		return FLIST
	case 'L':
		return LLIST
	case 'S':
		return STACK
	case 'Q':
		return QUEUE
	case 'T':
		return TREE
	}
	return 0
}

func creates(op int) bool {
	switch op {
	case OpAPush, OpLPush, OpLPushH, OpSPush, OpQPush, OpTInsert:
		return true
	}
	return false
}

//списки / стек / очередь 
func (c *Container) dbl() bool { return c.Kind == LLIST }

func (c *Container) find(v string) *Node {
	for n := c.Head; n != nil; n = n.Next {
		if n.Val == v {
			return n
		}
	}
	return nil
}

// предшественник
func (c *Container) pred(n *Node) *Node {
	if c.dbl() {
		return n.Prev
	}
	var p *Node
	for x := c.Head; x != nil && x != n; x = x.Next {
		p = x
	}
	return p
}

// вставка после p
func (c *Container) insertAfter(p *Node, v string) {
	n := &Node{Val: v}
	if p == nil {
		n.Next = c.Head
		if c.dbl() && c.Head != nil {
			c.Head.Prev = n
		}
		c.Head = n
		if c.Tail == nil {
			c.Tail = n
		}
	} else {
		n.Next = p.Next
		if c.dbl() {
			n.Prev = p
			if p.Next != nil {
				p.Next.Prev = n
			}
		}
		p.Next = n
		if c.Tail == p {
			c.Tail = n
		}
	}
	c.Size++
}

func (c *Container) erase(n *Node) {
	p := c.pred(n)
	if p != nil {
		p.Next = n.Next
	} else {
		c.Head = n.Next
	}
	if n.Next != nil {
		if c.dbl() {
			n.Next.Prev = p
		}
	} else {
		c.Tail = p
	}
	c.Size--
}

//Complete Binary Tree
func (c *Container) insertT(v string) {
	n := &TNode{Val: v}
	if c.Root == nil {
		c.Root = n
	} else {
		q := []*TNode{c.Root}
		for len(q) > 0 {
			x := q[0]
			q = q[1:]
			if x.Left == nil {
				x.Left = n
				break
			}
			q = append(q, x.Left)
			if x.Right == nil {
				x.Right = n
				break
			}
			q = append(q, x.Right)
		}
	}
	c.Size++
}

func findT(r *TNode, v string) *TNode {
	if r == nil {
		return nil
	}
	if r.Val == v {
		return r
	}
	if x := findT(r.Left, v); x != nil {
		return x
	}
	return findT(r.Right, v)
}

// обход в ширину после первого nil
func isComplete(root *TNode) bool {
	if root == nil {
		return true
	}
	q := []*TNode{root}
	gap := false
	for len(q) > 0 {
		x := q[0]
		q = q[1:]
		if x == nil {
			gap = true
			continue
		}
		if gap {
			return false
		}
		q = append(q, x.Left, x.Right)
	}
	return true
}

func levelOrder(root *TNode) []string {
	var out []string
	if root == nil {
		return out
	}
	q := []*TNode{root}
	for len(q) > 0 {
		x := q[0]
		q = q[1:]
		out = append(out, x.Val)
		if x.Left != nil {
			q = append(q, x.Left)
		}
		if x.Right != nil {
			q = append(q, x.Right)
		}
	}
	return out
}

// обходы в глубину
func walk(n *TNode, mode int, o *[]string) {
	if n == nil {
		return
	}
	if mode == 0 {
		*o = append(*o, n.Val)
	}
	walk(n.Left, mode, o)
	if mode == 1 {
		*o = append(*o, n.Val)
	}
	walk(n.Right, mode, o)
	if mode == 2 {
		*o = append(*o, n.Val)
	}
}

func printT(n *TNode, d int, sb *strings.Builder) {
	if n == nil {
		return
	}
	printT(n.Right, d+1, sb)
	sb.WriteString(strings.Repeat(" ", d*4) + n.Val + "\n")
	printT(n.Left, d+1, sb)
}

switch c.Kind {
	case ARR:
		return "[" + strings.Join(c.Arr, ", ") + "]"
	case FLIST, LLIST:
		sep := " -> "
		if c.Kind == LLIST {
			sep = " <-> "
		}
		var parts []string
		for n := c.Head; n != nil; n = n.Next {
			parts = append(parts, n.Val)
		}
		if len(parts) == 0 {
			return "(empty)"
		}
		return strings.Join(parts, sep)
	case STACK, QUEUE:
		if c.Head == nil {
			return "(empty)"
		}
		s := "front: "
		if c.Kind == STACK {
			s = "top: "
		}
		for n := c.Head; n != nil; n = n.Next {
			s += n.Val + " "
		}
		return s
	case TREE:
		var sb strings.Builder
		printT(c.Root, 0, &sb)
		if sb.Len() == 0 {
			return "(empty)"
		}
		return strings.TrimRight(sb.String(), "\n")
	}
	return ""
}

//сериализация
func ser(n *TNode, o *[]string) { // preorder, "#" = пустое место
	if n == nil {
		*o = append(*o, "#")
		return
	}
	*o = append(*o, n.Val)
	ser(n.Left, o)
	ser(n.Right, o)
}

func deser(t []string, i *int, cnt *int) *TNode {
	if *i >= len(t) || t[*i] == "#" {
		*i++
		return nil
	}
	n := &TNode{Val: t[*i]}
	*i++
	*cnt++
	n.Left = deser(t, i, cnt)
	n.Right = deser(t, i, cnt)
	return n
}

func toTokens(c *Container) []string {
	var o []string
	switch c.Kind {
	case ARR:
		return c.Arr
	case TREE:
		ser(c.Root, &o)
	default:
		for n := c.Head; n != nil; n = n.Next {
			o = append(o, n.Val)
		}
	}
	return o
}

func saveDb(db Db, file string) {
	var sb strings.Builder
	for name, c := range db {
		sb.WriteString(string("MFLSQT"[c.Kind-1]) + " " + name)
		for _, tok := range toTokens(c) {
			sb.WriteString(" " + tok)
		}
		sb.WriteString("\n")
	}
	os.WriteFile(file, []byte(sb.String()), 0644)
}

func loadDb(db Db, file string) bool {
	f, err := os.Open(file)
	if err != nil {
		return false
	}
	defer f.Close()
	sc := bufio.NewScanner(f)
	sc.Buffer(make([]byte, 1<<20), 1<<26)
	for sc.Scan() {
		w := strings.Fields(sc.Text())
		if len(w) < 2 {
			continue
		}
		c := &Container{Kind: kindOf(w[0][0])}
		db[w[1]] = c
		tokens := w[2:]
		switch c.Kind {
		case ARR:
			c.Arr = append([]string{}, tokens...)
		case TREE:
			i := 0
			c.Root = deser(tokens, &i, &c.Size)
		default:
			for _, v := range tokens {
				c.insertAfter(c.Tail, v)
			}
		}
	}
	return true
}

func idx(s string, lim int) (int, bool) {
	i, err := strconv.Atoi(s)
	return i, err == nil && i >= 0 && i < lim
}

func boolStr(b bool) string {
	if b {
		return "TRUE"
	}
	return "FALSE"
}

// выполнение запроса 
func execute(db Db, q string) string {
	t := strings.Fields(q)
	if len(t) < 2 {
		return "ERR: usage: COMMAND name [args]"
	}
	op, ok := table[t[0]]
	if !ok {
		return "ERR: unknown command " + t[0]
	}
	name, a := t[1], t[2:]

	c, exists := db[name]
	if !exists {
		if !creates(op) {
			return "ERR: no such container: " + name
		}
		c = &Container{Kind: kindOf(t[0][0])}
		db[name] = c
	}
	generic := t[0] == "PRINT" || t[0] == "ISMEMBER"
	if !generic && c.Kind != kindOf(t[0][0]) {
		return "ERR: wrong container type"
	}
	if t[0] == "ISMEMBER" && c.Kind != TREE {
		return "ERR: not a tree"
	}

	switch op {
	//массив
	case OpAPush:
		if len(a) < 1 {
			return errArgs
		}
		i := len(c.Arr)
		if len(a) > 1 {
			var good bool
			if i, good = idx(a[1], len(c.Arr)+1); !good {
				return errRange
			}
		}
		c.Arr = append(c.Arr, "")
		copy(c.Arr[i+1:], c.Arr[i:])
		c.Arr[i] = a[0]
		return a[0]
	case OpADel:
		if len(a) < 1 {
			return errArgs
		}
		i, good := idx(a[0], len(c.Arr))
		if !good {
			return errRange
		}
		v := c.Arr[i]
		c.Arr = append(c.Arr[:i], c.Arr[i+1:]...)
		return v
	case OpAGet:
		if len(a) < 1 {
			return errArgs
		}
		i, good := idx(a[0], len(c.Arr))
		if !good {
			return errRange
		}
		return c.Arr[i]
	case OpASet:
		if len(a) < 2 {
			return errArgs
		}
		i, good := idx(a[0], len(c.Arr))
		if !good {
			return errRange
		}
		c.Arr[i] = a[1]
		return a[1]
	case OpALen:
		return strconv.Itoa(len(c.Arr))

	// ----- односвязный / двусвязный список -----
	case OpLPush:
		if len(a) < 1 {
			return errArgs
		}
		c.insertAfter(c.Tail, a[0])
		return a[0]
	case OpLPushH:
		if len(a) < 1 {
			return errArgs
		}
		c.insertAfter(nil, a[0])
		return a[0]
	case OpLPushB:
		if len(a) < 2 {
			return errArgs
		}
		n := c.find(a[0])
		if n == nil {
			return errNF
		}
		c.insertAfter(c.pred(n), a[1])
		return a[1]
	case OpLPushA:
		if len(a) < 2 {
			return errArgs
		}
		n := c.find(a[0])
		if n == nil {
			return errNF
		}
		c.insertAfter(n, a[1])
		return a[1]
	case OpLDel:
		if len(a) < 1 {
			return errArgs
		}
		n := c.find(a[0])
		if n == nil {
			return errNF
		}
		c.erase(n)
		return "OK"
	case OpLDelH:
		if c.Head == nil {
			return errEmpty
		}
		c.erase(c.Head)
		return "OK"
	case OpLDelT:
		if c.Tail == nil {
			return errEmpty
		}
		c.erase(c.Tail)
		return "OK"
	case OpLDelB:
		if len(a) < 1 {
			return errArgs
		}
		n := c.find(a[0])
		if n == nil {
			return errNF
		}
		p := c.pred(n)
		if p == nil {
			return "ERR: no element before"
		}
		c.erase(p)
		return "OK"
	case OpLDelA:
		if len(a) < 1 {
			return errArgs
		}
		n := c.find(a[0])
		if n == nil {
			return errNF
		}
		if n.Next == nil {
			return "ERR: no element after"
		}
		c.erase(n.Next)
		return "OK"
	case OpLFind:
		if len(a) < 1 {
			return errArgs
		}
		i := 0
		for n := c.Head; n != nil; n = n.Next {
			if n.Val == a[0] {
				return "TRUE (index " + strconv.Itoa(i) + ")"
			}
			i++
		}
		return "FALSE"
	case OpLGet:
		return show(c)
	case OpLGetI: // элемент по индексу
		if len(a) < 1 {
			return errArgs
		}
		i, good := idx(a[0], c.Size)
		if !good {
			return errRange
		}
		n := c.Head
		for ; i > 0; i-- {
			n = n.Next
		}
		return n.Val
	case OpLGetR: // только двусвязный
		var parts []string
		for n := c.Tail; n != nil; n = n.Prev {
			parts = append(parts, n.Val)
		}
		if len(parts) == 0 {
			return "(empty)"
		}
		return strings.Join(parts, " <-> ")

	//стек и очередь
	case OpSPush:
		if len(a) < 1 {
			return errArgs
		}
		c.insertAfter(nil, a[0])
		return a[0]
	case OpQPush:
		if len(a) < 1 {
			return errArgs
		}
		c.insertAfter(c.Tail, a[0])
		return a[0]
	case OpSPop, OpQPop:
		if c.Head == nil {
			return errEmpty
		}
		v := c.Head.Val
		c.erase(c.Head)
		return v
	case OpSPeek, OpQPeek:
		if c.Head == nil {
			return errEmpty
		}
		return c.Head.Val

	// CBT
	case OpTInsert:
		if len(a) < 1 {
			return errArgs
		}
		if a[0] == "#" {
			return "ERR: '#' is reserved"
		}
		c.insertT(a[0])
		return a[0]
	case OpTAdd:
		if len(a) < 3 {
			return errArgs
		}
		if a[2] == "#" || (a[1] != "L" && a[1] != "R") {
			return "ERR: bad arguments"
		}
		p := findT(c.Root, a[0])
		if p == nil {
			return errNF
		}
		slot := &p.Right
		if a[1] == "L" {
			slot = &p.Left
		}
		if *slot != nil {
			return "ERR: slot occupied"
		}
		*slot = &TNode{Val: a[2]}
		c.Size++
		return a[2]
	case OpTSearch:
		if len(a) < 1 {
			return errArgs
		}
		return boolStr(findT(c.Root, a[0]) != nil)
	case OpTComplete:
		return boolStr(isComplete(c.Root))
	case OpTGet:
		if c.Root == nil {
			return "(empty)"
		}
		return strings.Join(levelOrder(c.Root), " ")
	case OpTPre, OpTIn, OpTPost:
		mode := 0
		switch op {
		case OpTIn:
			mode = 1
		case OpTPost:
			mode = 2
		}
		var o []string
		walk(c.Root, mode, &o)
		if len(o) == 0 {
			return "(empty)"
		}
		return strings.Join(o, " ")

	case OpPrint:
		return show(c)
	}
	return "ERR: unsupported"
}
