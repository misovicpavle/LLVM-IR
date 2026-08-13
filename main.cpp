#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <functional>

// Core LLVM IR Headers
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/NoFolder.h>



//LLVM
/*
 NO BIT NARROWING(62-32) AND NO SIGN BIT CONVERSIONS(signed-unsigned)
 */

/*
 * Statement node int a = 1; -> returns void / *Value = &nullptr
 * AST - done by hand in main(), you connect nodes and pass codegen(RootNode) for every line of code you want to parse
 * codegen() simply return a result
 *
 */



class Node
{
    protected:
        bool isSigned {false};
    public:
        Node() = default;
        virtual ~Node() = default;
        explicit Node(bool isSigned) : isSigned(isSigned) {}
        [[nodiscard]] virtual bool getSigned() const { return false; }
        virtual llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) = 0; //pure virtual
};

enum BinaryNodeType {
    MUL, DIV, ADD, SUB
};

class LeafIntNode : public Node
{
    protected:
        std::int64_t value{0};
    public:
        LeafIntNode() = default;
        explicit LeafIntNode(int value) : value(value) {}
        LeafIntNode(std::uint32_t value) : value(value) {}
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            return llvm::ConstantInt::get(builder.getInt32Ty(), value);
        }
};


class LeafDoubleNode : public Node
{
    protected:
        double value{0.0};
    public:
        LeafDoubleNode() = default;
        explicit LeafDoubleNode(float) : value(value) {}
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            return llvm::ConstantFP::get(builder.getDoubleTy(), value);
        }
};


class LeafFloatNode : public Node
{
    protected:
        float value{0.0f};
    public:
        LeafFloatNode() = default;
        LeafFloatNode(float value) : value(value) {}
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            return llvm::ConstantFP::get(builder.getFloatTy(), value);
        }
};


class LeafBoolNode : public Node //1bit int
{
    protected:
        bool value{false};
    public:
        LeafBoolNode() = default;
        LeafBoolNode(bool value) : value(value) {}
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            return llvm::ConstantFP::get(builder.getInt1Ty(), value); //retrieve 1 bit int
        }
};


class LeafStringNode : public Node {
    protected:
        std::string value{""};
    public:
        LeafStringNode() = default;
        LeafStringNode(std::string value) : value(std::move(value)) {}
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            return builder.CreateGlobalString(value); //evaluates to int 8b*
        }
};

enum ComparisonOperator { GREATER, LESS, EQUAL, NOTEQUAL, GREATER_EQUAL, LESS_EQUAL };

class ComparisonNode : public Node {
protected:
    ComparisonOperator op { ComparisonOperator::GREATER };
    std::unique_ptr<Node> lhs; //4 - Node, or it can be (4*123) - BinaryNode: *, children: 4, 123
    std::unique_ptr<Node> rhs;
public:
    ComparisonNode() = default;
    ComparisonNode(ComparisonOperator op, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : op(op), lhs(std::move(rhs)), rhs(std::move(rhs)) {}
    llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
        auto const left = lhs->codegen(builder, context);
        auto const right = rhs->codegen(builder, context);

        //pointer validation
        if (left == nullptr || right == nullptr) throw std::runtime_error("Invalid comparison node");
        if (left->getType() != right->getType()) throw std::runtime_error("Type mismatch in comparison node");

        if (left->getType()->isIntegerTy()) {
            //for integers
            if (lhs->getSigned() != rhs->getSigned()) throw std::runtime_error("Signedness mismatch in comparison node");
            if (lhs->getSigned()) { //is signed?
                if (op == ComparisonOperator::GREATER) return builder.CreateICmpSGT(left, right);
                if (op == ComparisonOperator::LESS) return builder.CreateICmpSLT(left, right);
                if (op == ComparisonOperator::EQUAL) return builder.CreateICmpEQ(left, right);
                if (op == ComparisonOperator::NOTEQUAL) return builder.CreateICmpNE(left, right);
                if (op == ComparisonOperator::GREATER_EQUAL) return builder.CreateICmpSGE(left, right);
                if (op == ComparisonOperator::LESS_EQUAL) return builder.CreateICmpSLE(left, right);
            }
            else {
                if (op == ComparisonOperator::GREATER) return builder.CreateICmpUGT(left, right);
                if (op == ComparisonOperator::LESS) return builder.CreateICmpULT(left, right);
                if (op == ComparisonOperator::EQUAL) return builder.CreateICmpEQ(left, right);
                if (op == ComparisonOperator::NOTEQUAL) return builder.CreateICmpNE(left, right);
                if (op == ComparisonOperator::GREATER_EQUAL) return builder.CreateICmpUGE(left, right);
                if (op == ComparisonOperator::LESS_EQUAL) return builder.CreateICmpULE(left, right);
            }
        }
        else if (left->getType()->isFloatingPointTy()) {
            //for floats
            if (op == ComparisonOperator::GREATER) return builder.CreateFCmpOGT(left, right);
            if (op == ComparisonOperator::LESS) return builder.CreateFCmpOLT(left, right);
            if (op == ComparisonOperator::EQUAL) return builder.CreateFCmpOEQ(left, right);
            if (op == ComparisonOperator::NOTEQUAL) return builder.CreateFCmpONE(left, right);
            if (op == ComparisonOperator::GREATER_EQUAL) return builder.CreateFCmpOGE(left, right);
            if (op == ComparisonOperator::LESS_EQUAL) return builder.CreateFCmpOLE(left, right);
        }
        throw std::runtime_error("Unsupported comparison operator");
    }
};

//if there are terminators from the ->getTerminator(), they should specifically say where to jump to


/*
FULL BLOWN METHOD HANDLING:
class CodeGenContext
{
    protected:
        llvm::LLVMContext context;
        llvm::IRBuilder<> builder;
        std::vector<block> compilerState;
    public:
        auto getBuilder() { return &builder; }
        auto getContext() { return &context; }
        auto getCompilerState() { return &compilerState; }
        CodeGenContext() : context(), builder(context) {}
};
*/

/*
 *llvm::Value* codegen(CodeGenContext& ctx) override {
auto builder = ctx.getBuilder();
        auto context = ctx.getContext();
        auto function = builder->GetInsertBlock()->getParent();
 */

/*
getTerminator() retunrs nullptr by default:
it checks the llvm::instruction of a llvm:basicblock and its list of instructions, and looks at the most recent one

 */

struct LoopTargets {
    inline static std::vector<llvm::BasicBlock*> exitBlocks;
    inline static std::vector<llvm::BasicBlock*> continueBlocks;
};

class breakNode : public Node //break leaves the loop aka switches from bodyBlock to exitBlock
{
protected:
public:
    llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
        if (LoopTargets::exitBlocks.empty()) throw std::runtime_error("No loop to break from");
        builder.CreateBr(LoopTargets::exitBlocks.back());
        return nullptr;
    }
};

class continueNode : public Node //continue switches from the bodyBlock to the conditonBlock
{
protected:
public:
    llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
        if (LoopTargets::continueBlocks.empty()) throw std::runtime_error("No loop to continue from");
        builder.CreateBr(LoopTargets::continueBlocks.back());
        return nullptr;
    }
};

//this node is for full-blown method handling - not yet implemented
class returnNode : public Node //return terminates the function its called in completely and gives a rvalue
{
protected:
public:
    llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
        //return builder.CreateBr();
    }
};


class IfNode : public Node {
    protected:
        std::unique_ptr<Node> thenBody;
        std::unique_ptr<Node> elseBody;
        std::unique_ptr<Node> condition; // if(lhs > rhs): >,<,<=,>=,==,!= - conditional operators - comparison node
    public:
        IfNode() = default;
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            auto const result = condition->codegen(builder, context);
            auto function = builder.GetInsertBlock()->getParent();
            auto thenBlock = llvm::BasicBlock::Create(context, "then", function);
            auto elseBlock = llvm::BasicBlock::Create(context, "else");
            auto mergeBlock = llvm::BasicBlock::Create(context, "merge");
            //LLVM SSA single static assignment means no overwriting variables accross branches
            //PHI acts as a conditional selector, if execution came from A, select A, else B
            builder.CreateCondBr(result, thenBlock, elseBlock);
            builder.SetInsertPoint(thenBlock); //any instructions after this line will be executed for the then block, everything goes into the thenBlock
            llvm::Value* thenValue = nullptr;
            if (thenBody != nullptr) thenValue = thenBody->codegen(builder, context);
            if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBlock); //end of then - merge its results
            auto thenEndBlock = builder.GetInsertBlock(); //has to be here, then might have nested if's...
            llvm::BasicBlock* elseEndBlock = nullptr;
            llvm::Value* elseValue = nullptr;
            function->insert(function->end(), elseBlock);
            builder.SetInsertPoint(elseBlock); //time for else
            if (elseBody != nullptr) {
                elseValue = elseBody->codegen(builder, context);
                if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBlock);
                elseEndBlock = builder.GetInsertBlock();
            }
            else { // createcondbr NEEDS the else block to be something, if no else exists - crash
                builder.CreateBr(mergeBlock);
                elseEndBlock = builder.GetInsertBlock();
            }
            function->insert(function->end(), mergeBlock);
            builder.SetInsertPoint(mergeBlock);
            //phinode must have both operands not null
            llvm::PHINode* phi = nullptr;
            if (thenValue == nullptr || elseValue == nullptr) return nullptr;
            else {
                if (thenValue->getType() == elseValue->getType()) {
                    phi = builder.CreatePHI(thenValue->getType(), 2, "result"); //2 possible results
                    phi->addIncoming(thenValue, thenEndBlock);
                    phi->addIncoming(elseValue, elseEndBlock);
                }
                else throw std::runtime_error("Type mismatch in phi node");
            }
            return phi;
        }
};


class whileLoopNode : public Node {
    protected:
        std::unique_ptr<Node> condition;
        std::unique_ptr<Node> body;
    public:
        whileLoopNode() = default;
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            //block creation conditionBlock -> bodyBlock -> conditionBlock... -> exitBlock
            auto function = builder.GetInsertBlock()->getParent();
            auto conditionBlock = llvm::BasicBlock::Create(context, "condition");
            auto bodyBlock = llvm::BasicBlock::Create(context, "body");
            auto exitBlock = llvm::BasicBlock::Create(context, "exit"); //no function here because this needs to be regulated when it happens
            //reset point
            function->insert(function->end(), conditionBlock);
            builder.CreateBr(conditionBlock); //unconditional jump to condition
            builder.SetInsertPoint(conditionBlock);
            llvm::Value* conditionValue = nullptr;
            llvm::Value* bodyValue = nullptr;
            llvm::BasicBlock* exitPoint = nullptr;
            if (condition != nullptr) {
                conditionValue = condition->codegen(builder, context);
                if (conditionValue->getType() != builder.getInt1Ty()) throw std::runtime_error("Condition must be boolean");
                builder.CreateCondBr(conditionValue, bodyBlock, exitBlock);
            }
            else throw std::runtime_error("While loop condition is null");
            // body
            function->insert(function->end(), bodyBlock);
            builder.SetInsertPoint(bodyBlock);
            LoopTargets::exitBlocks.push_back(exitBlock);
            LoopTargets::continueBlocks.push_back(conditionBlock);
            if (body != nullptr) bodyValue = body->codegen(builder, context);
            if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(conditionBlock); //terminators: ret, condbr,switch..
            //exit point
            LoopTargets::exitBlocks.pop_back();
            LoopTargets::continueBlocks.pop_back();
            function->insert(function->end(), exitBlock);
            builder.SetInsertPoint(exitBlock);

            return nullptr;
        }
};


class doWhileLoopNode : public Node {
    protected:
        std::unique_ptr<Node> condition;
        std::unique_ptr<Node> body;
    public:
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            auto function = builder.GetInsertBlock()->getParent();
            auto conditionBlock = llvm::BasicBlock::Create(context, "condition");
            auto bodyBlock = llvm::BasicBlock::Create(context,"body");
            auto exitBlock = llvm::BasicBlock::Create(context, "exit");
            llvm::Value* conditionValue = nullptr;
            llvm::Value* bodyValue = nullptr;
            function->insert(function->end(), bodyBlock);
            builder.CreateBr(bodyBlock);
            builder.SetInsertPoint(bodyBlock);
            LoopTargets::exitBlocks.push_back(exitBlock);
            LoopTargets::continueBlocks.push_back(conditionBlock);
            if (body != nullptr) bodyValue = body->codegen(builder, context);
            if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(conditionBlock); //if there is not a terminator, !nullptr=true so it jumps
            LoopTargets::exitBlocks.pop_back();
            LoopTargets::continueBlocks.pop_back();
            function->insert(function->end(), conditionBlock);
            builder.SetInsertPoint(conditionBlock);
            if (condition != nullptr) {
                conditionValue = condition->codegen(builder, context);
                if (conditionValue->getType() != builder.getInt1Ty()) throw std::runtime_error("Condition must be boolean");
                builder.CreateCondBr(conditionValue, bodyBlock, exitBlock);
            }
            else throw std::runtime_error("Do-While loop condition is null");
            function->insert(function->end(), exitBlock);
            builder.SetInsertPoint(exitBlock);

            return nullptr;
        }
};


class forLoopNode : public Node {
    protected:
        std::unique_ptr<Node> init;
        std::unique_ptr<Node> condition;
        std::unique_ptr<Node> increment;
        std::unique_ptr<Node> body;
    public:
        forLoopNode() = default;
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            //block creation conditionBlock -> bodyBlock -> conditionBlock... -> exitBlock
            auto function = builder.GetInsertBlock()->getParent();
            auto conditionBlock = llvm::BasicBlock::Create(context, "condition");
            auto bodyBlock = llvm::BasicBlock::Create(context, "body");
            auto incrementBlock = llvm::BasicBlock::Create(context, "increment");
            auto exitBlock = llvm::BasicBlock::Create(context, "exit"); //no function here because this needs to be regulated when it happens
            //reset point
            llvm::Value* initValue = nullptr;
            llvm::Value* conditionValue = nullptr;
            llvm::Value* bodyValue = nullptr;
            llvm::BasicBlock* exitPoint = nullptr;
            if (init != nullptr) initValue = init->codegen(builder, context);
            function->insert(function->end(), conditionBlock);
            builder.CreateBr(conditionBlock);
            builder.SetInsertPoint(conditionBlock);
            if (condition != nullptr) {
                conditionValue = condition->codegen(builder, context);
                if (conditionValue->getType() != builder.getInt1Ty()) throw std::runtime_error("Condition must be boolean");
                builder.CreateCondBr(conditionValue, bodyBlock, exitBlock);
            }
            else builder.CreateBr(bodyBlock);
            // body
            function->insert(function->end(), bodyBlock);
            builder.SetInsertPoint(bodyBlock);
            LoopTargets::exitBlocks.push_back(exitBlock);
            LoopTargets::continueBlocks.push_back(incrementBlock);
            if (body != nullptr) bodyValue = body->codegen(builder, context);
            if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(incrementBlock); //terminators: ret, condbr,switch..
            LoopTargets::exitBlocks.pop_back();
            LoopTargets::continueBlocks.pop_back();
            //increment block
            function->insert(function->end(), incrementBlock);
            builder.SetInsertPoint(incrementBlock);
            if (increment != nullptr) {
                increment->codegen(builder, context);
            }
            if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(conditionBlock); //go back to condition block
            //exit point
            function->insert(function->end(), exitBlock);
            builder.SetInsertPoint(exitBlock);

            return nullptr;
        }
};


class BinaryNode : public Node {
    protected:
        BinaryNodeType type{BinaryNodeType::ADD};
        std::vector<std::unique_ptr<Node>> children;
    public:
        BinaryNode(BinaryNodeType type, std::vector<std::unique_ptr<Node>> children) : type(type), children(std::move(children)) {}
        BinaryNode() = default;
        llvm::Value* codegen(llvm::IRBuilderBase& builder, llvm::LLVMContext& context) override {
            //std::cout << "BinaryNode children count: " << children.size() << std::endl; // Should print 2
            // (lhs) BinaryNode (rhs)  <==>  (lhs) (*,+,-,/...) (rhs)
            auto* lhs = children[0]->codegen(builder, context); //ptr to Value*(aka ptr to ptr of Value where Value is the value of the virtual register)
            auto* rhs = children[1]->codegen(builder, context);
            /* Floating point - CreateFAdd, CreateFSub, CreateFMul, CreateFDiv(no matter if signed/unsigned) */
            /* Integer - CreateAdd, CreateSub, CreateMul, CreateSDiv(signed), CreateUDiv(unsigned) */
            bool sameLLVMtype = (lhs->getType() == rhs->getType());
            bool sameSignedness =  (children[0]->getSigned() == children[1]->getSigned());
            if (sameLLVMtype && lhs->getType()->isFloatingPointTy()) { //64bit check instead of isFloatTy()
                if (type == BinaryNodeType::ADD) return builder.CreateFAdd(lhs, rhs);
                if (type == BinaryNodeType::SUB) return builder.CreateFSub(lhs, rhs);
                if (type == BinaryNodeType::MUL) return builder.CreateFMul(lhs, rhs);
                if (type == BinaryNodeType::DIV) return builder.CreateFDiv(lhs, rhs);
            }
            else if ((sameLLVMtype && sameSignedness) && lhs->getType()->isIntegerTy()) {
                //this getType catched bit width, float precisions(double or float) and data type mismatch
                //but it misses signed vs unsigned - uint32_t & int32_t same i32 type pointer in memory
                if (type == BinaryNodeType::ADD) return builder.CreateAdd(lhs, rhs);
                if (type == BinaryNodeType::SUB) return builder.CreateSub(lhs, rhs);
                if (type == BinaryNodeType::MUL) return builder.CreateMul(lhs, rhs);
                if (type == BinaryNodeType::DIV) {
                    if (children[0]->getSigned() && children[1]->getSigned()) {
                        return builder.CreateSDiv(lhs, rhs);
                    }
                    else {
                        return builder.CreateUDiv(lhs, rhs);
                    }
                }
            }
            else if (!sameLLVMtype || !sameSignedness){ //lhs/rhs are different types - promotion/narrowing conversions ahead
                throw std::runtime_error("Type policing error: Implicit conversions for sign bits and data types are disabled.");
            }
            throw std::runtime_error("Unsupported or mismatched operand types in BinaryNode");
        }
};

/*
 *
 * LLVM IR requires a strict container hierarchy:
 * Module (Top Level) - Contains Functions
 * Function (e.g., main) - Contains Basic Blocks (labeled code regions like entry:)
 * Basic Block - Contains Instructions (the actual IR instructions generated by AST
 */

void printLLVMIR(const std::vector<std::unique_ptr<Node>> &ast, llvm::IRBuilderBase& builder, llvm::LLVMContext& context, llvm::Module& module) {
    //no smart pointers - module manages its own memory
    //std::cout << "ast size: " << ast.size() << std::endl;
    llvm::Function* mainFunc = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), false), llvm::Function::ExternalLinkage, "main", &module);
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entry); //builder now attaches all incoming ast instructions to this function block
    llvm::Value* result = nullptr;
    for (const auto& rootNode : ast)
        result = rootNode->codegen(builder, context);
    builder.CreateRetVoid(); // set a terminator so verifymodule doesnt flag and fail
    llvm::verifyModule(module); //check for errors before printing
    module.print(llvm::outs(), nullptr);
    //module.dump(); //debugging thing - dumps all textual IR to stderr, no need for stream parameters
}

// Value * -> virtual register that holds something (constant - 5.0, result of some expression - a + b etc..)
// it takes one node, and deconstructs it to its most derived children, and returns the result by *Value - which is



int main()
{
    //MANUAL TEST - MANUAL AST INPUT & CONNECT
    llvm::LLVMContext context;
    llvm::IRBuilder<llvm::NoFolder> builder(context);
    llvm::Module module("test", context);

    std::vector<std::unique_ptr<Node>> ast; //root nodes
    std::vector<std::unique_ptr<Node>> children1;
    std::vector<std::unique_ptr<Node>> children2;
    children1.push_back(std::make_unique<LeafIntNode>(1));
    children1.push_back(std::make_unique<LeafIntNode>(2));
    children2.push_back(std::make_unique<LeafDoubleNode>(3.23));
    children2.push_back(std::make_unique<LeafDoubleNode>(4.23));

    /*
     EXAMPLE VISUALISATION:

                  x                                  y
                  | =                                | =
                root1                              root2
            |-----|-----|                      |-----|-----|
            1     +     2                     3.23   *    4.23

            int x = 1 + 2;

            double y = 3.23 * 4.23;

            // if (x > y) print("Hey!");

            LLVM IR: ???
    */
    // Instantiate and move directly into the ast vector

    ast.push_back(std::make_unique<BinaryNode>(BinaryNodeType::ADD, std::move(children1)));
    ast.push_back(std::make_unique<BinaryNode>(BinaryNodeType::MUL, std::move(children2)));

    printLLVMIR(ast, builder, context, module);
    return 0;
}