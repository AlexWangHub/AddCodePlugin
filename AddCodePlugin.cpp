#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include <iostream>

using namespace std;
using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

static cl::OptionCategory ObfOptionCategory("ObfOptionCategory");


// 匹配回调
class MatchCallbackHandler : public  MatchFinder::MatchCallback {
public:
    //构造函数
    MatchCallbackHandler(CompilerInstance *aCompilerInstance):compilerInstance(aCompilerInstance)  {}
    virtual void run(const MatchFinder::MatchResult &Result) {
        const ObjCMessageExpr *objCMessageExpr          = Result.Nodes.getNodeAs<ObjCMessageExpr>("objCMessageExpr");
        if (objCMessageExpr) {
            string selectorName = objCMessageExpr->getSelector().getAsString();
            if (selectorName == "viewDidLoad") {
                cout << "危险方法被调用："<< selectorName << endl;
            }
        }
        
    }
private:
    CompilerInstance *compilerInstance;
};

//AST 构造器
class ObfASTConsumer : public ASTConsumer {
public:
    ObfASTConsumer(CompilerInstance *aCI) :handlerMatchCallback(aCI)  {
        //添加匹配器
        matcher.addMatcher(objcMessageExpr().bind("objCMessageExpr"),&handlerMatchCallback);
    }

    void HandleTranslationUnit(ASTContext &Context) override {
        //运行匹配器
        matcher.matchAST(Context);
    }
    
    private:
    MatchFinder matcher;
    MatchCallbackHandler handlerMatchCallback;

};

//action
class ObfASTFrontendAction : public ASTFrontendAction {
public:
    //创建AST Consumer
    std::unique_ptr<ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, StringRef file) override {
        return std::make_unique<ObfASTConsumer>(&CI);
    }
    void EndSourceFileAction() override {
        cout << "处理完成" << endl;
    }
};

//入口main函数
int main(int argc, const char **argv) {
    CommonOptionsParser OptionsParser(argc, argv, ObfOptionCategory);
    ClangTool Tool(OptionsParser.getCompilations(),OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<ObfASTFrontendAction>().get());
}
