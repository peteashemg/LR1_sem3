package main

//коды типов контейнеров
const (
	ARR = iota + 1
	FLIST
	LLIST
	STACK
	QUEUE
	TREE
)

//коды команд
const (
	OpAPush = iota + 1
	OpADel
	OpAGet
	OpASet
	OpALen
	OpLPush
	OpLPushH
	OpLPushB
	OpLPushA
	OpLDel
	OpLDelH
	OpLDelT
	OpLDelB
	OpLDelA
	OpLFind
	OpLGet
	OpLGetR
	OpLGetI
	OpSPush
	OpSPop
	OpSPeek
	OpQPush
	OpQPop
	OpQPeek
	OpTInsert
	OpTAdd
	OpTSearch
	OpTComplete
	OpTGet
	OpTPre
	OpTIn
	OpTPost
	OpPrint
)

//структура списка / стека / очереди 
type Node struct {
	Val        string
	Next, Prev *Node
}

//Complete Binary Tree
type TNode struct {
	Val         string
	Left, Right *TNode
}

type Container struct {
	Kind       int
	Arr        []string
	Head, Tail *Node
	Root       *TNode
	Size       int
}

type Db map[string]*Container
