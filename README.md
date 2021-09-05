# Clang命令行工具开发

作为iOS开发，clang和LLVM我们应该都挺熟悉了，最近打算用Clang制作一个代码扫描的小工具，把Clang命令行工具开发的流程记录于此。

## 前言

Clang 作为前端编译器，对iOS源代码进行词法、语法的分析，实现"万千代码，终归一体"的效果，让我们可以从词法、语法的角度实现对工程质量的保证。

我制作Clang命令行工具的初衷有两个：

- 业务代码定制扫描，提供工程代码质量，比如检测项目里危险代码调用方、敏感代码调用和成对代码调用等。
- 代码自动化生成，提升开发效率，避免不必要的犯错，比如PBCoding代码生成。

## 文章大纲

经过我自己的摸索，我把学习 Clang命令行工具的步骤用下面5个步骤概括：

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu69fl7a0zj60c40gzdgy02.jpg)

先备知识中，如果你能看懂下面这幅图表达的含义，那么可以说是具备iOS工程编译流程的知识储备的：

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu68ls3rd2j60vm0enta502.jpg)

如果你想再验证一下自己掌握的iOS工程编译流程知识是否扎实，可以试着回答这样几个问题：

- 如果让你动手制作一个 framework ，你会怎么做，有哪些条件是开始制作 framework 前要商定好的？
- 什么情况下，framework 开发只能给 二进制包，而不能使用 submodule 协同开发？
- clang 是 LLVM 的子集吗？ clang 和 LLVM 是什么关系？
- Injection 等热加载工具的原理

这几个问题是我最近在iOS工程编译上遇到的问题，仅做记录，分享于此。

基于iOS工程编译流程的「先备知识」已经有过分享，所以这里我们从「环境搭建」开始，描述 Clang命令行工具开发的流程。

## 一、环境搭建

### 0. clone LLVM项目

clone llvm 项目工程：git clone https://github.com/llvm/llvm-project.git

### 1. 编译LLVM

在 llvm-project 项目中新建 build 目录，并cd build，使用cmake命令编译LLVM工程，生成Xcode可以编译的文件格式。

cmake -G Xcode -DLLVM_ENABLE_PROJECTS=clang ../llvm

### 2. 文件概览

查看 llvm-project 工程，重点可以看下 build、clang（前端编译器）、lld（链接器）、lldb（调试器）、llvm（optimization代码优化 + 生成平台相关的汇编代码）

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu68zz8l1rj611c0segpi02.jpg)

### 3. Xcode编译LLVM

打开 build - LLVM.Xcodeproj ，选择 Autocreat Schemes，添加 schemes All_BUILD，开始编译 

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu5rqxfhbhj60ac0bomxl02.jpg)

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu5rsdmyn0j60mn059q3l02.jpg)

cmd + B 开始进行编译，估计要编半小时

### 4. 生成结构概览

编译完成后，我们可以在 llvm-project - build - Debug - bin 看到编译生成的命令行工具

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu5t3s3gxvj60ln0fwtar02.jpg)

Tips:其中有一些是非常有用的，比如 clang-format 可以实现代码格式化。

### 5. 新建clang开发文件

下面开始通过 clang 构建工具，进入 llvm-project - clang - tools 

- a. 新建 AddCodePlugin 文件夹，在文件夹中添加 AddCodePlugin.cpp 和 CMakeLists.txt

Tips： AddCodePlugin.cpp 是我们编写工具代码的地方，CMakeLists.txt 是我们使用 CMake 编译时，添加依赖的文件

- b. 在 llvm-project - clang - tools 目录下 CMakeLists.txt 文件中新增 ： add_clang_subdirectory(AddCodePlugin)

### 6. 重新编译

编写完 AddCodePlugin.cpp ，配置好 CMakeLists.txt 后，我们开启重编：

返回 build 文件夹，执行 cmake -G Xcode -DLLVM_ENABLE_PROJECTS=clang ../llvm

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu5tayjvp4j60u00w8dja02.jpg)

如此，我们看到文件编译出来了。

### 7. Xcode打开项目

接着我们重新打开 LLVM.xcodeproj ，这时在 LLVM.xcodeproj 中已经能看到我们刚才新增的项目文件了：

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu5tcx11tbj609d07ot8w02.jpg)

到此，我们开发Clang命令行工具的环境已经准备完毕了，可以使用Xcode愉快地进入开发流程了。

## 二、开发框架选择

现在我们有了 AddCodePlugin.cpp 这个文件，接下来的问题是：我们怎么开发呢？iOS画UI有UIKit框架，新建对象有NSObject类，那我们开发Clang命令行工具有框架可以选择吗？ 

是有的。

一共有 LibClang（ClangKit）、LibTooling两个工具库可供你开发使用，下面是网上对这两个库的描述：

### LibClang

LibClang 提供了一个稳定的高级 C 接口，Xcode 使用的就是 LibClang。LibClang 可以访问 Clang 的上层高级抽象的能力，比如获取所有 Token、遍历语法树、代码补全等。由于 API 很稳定，Clang 版本更新对其影响不大。但是，LibClang 并不能完全访问到 Clang AST 信息。

使用 LibClang 可以直接使用它的 C API。官方也提供了 Python binding 脚本供你调用。还有开源的 node-js/ruby binding。你要是不熟悉其他语言，还有个第三方开源的 Objective-C 写的ClangKit 库可供使用。

### LibTooling

LibTooling 是一个 C++ 接口，通过 LibTooling 能够编写独立运行的语法检查和代码重构工具。LibTooling 的优势如下：

所写的工具不依赖于构建系统，可以作为一个命令单独使用，比如 clang-check、clang-fixit、clang-format；
可以完全控制 Clang AST，能够和 Clang Plugins 共用一份代码；

与 Clang Plugins 相比，LibTooling 无法影响编译过程；与 LibClang 相比，LibTooling 的接口没有那么稳定，也无法开箱即用，当 AST 的 API 升级后需要更新接口的调用。但是，LibTooling 基于能够完全控制 Clang AST 和可独立运行的特点，可以做的事情就非常多了。比如代码语言转换、坚持代码规范、分析甚至重构代码等。
在 LibTooling 的基础之上有个开发人员工具合集 Clang tools，Clang tools 作为 Clang 项目的一部分，已经提供了一些工具，主要包括：

- 语法检查工具 clang-check；
- 自动修复编译错误工具 clang-fixit；
- 自动代码格式工具 clang-format；
- 新语言和新功能的迁移工具；
- 重构工具。

网上介绍如此，但我自己的需求是针对 AST 进行操作，LibClang 不能完全访问到 Clang AST 的信息，担心会成为未来的一个瓶颈，且我调研后发现 LibTooling库 使用起来也挺方便，只是 AST 的 API 升级确实会造成代码变动的情况，但考虑到 Clang命令行 的使用场景是接受2-3天buffer对接新API的，所以最终决定使用 LibTooling 。

## 三、代码开发

### 1. 目标

新建一个 `callMethod.m`测试类，代码如下：

```
#import "ViewController.h"

@interface ViewController ()

@property (nonatomic, strong) NSString *test;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)hello {
    [self viewDidLoad];
    [self exit];
}

- (void)exit {
	
}

@end

```

你可能会觉得我这份测试类代码有点问题，怎么在 `hello`方法里主动调用了 `viewDidLoad`呢？

其实是故意这样处理的，我们的目标就是：编写 clang命令行工具，检测出主动调用了 `viewDidLoad` 的方法。

### 2. AST（Abstract Syntax Tree 抽象语法树） 结构

在具体开发Clang插件前，我们先来看看我们要分析的AST的结构是怎么样的，我们输入如下命令：

`clang -Xclang -ast-dump -fsyntax-only /Users/binbinwang/Desktop/callMethod.m`

发现生成了如下的AST：

```
#import "ViewController.h"
        ^~~~~~~~~~~~~~~~~~
TranslationUnitDecl 0x7fd098037608 <<invalid sloc>> <invalid sloc>
|-TypedefDecl 0x7fd098037ed8 <<invalid sloc>> <invalid sloc> implicit __int128_t '__int128'
| `-BuiltinType 0x7fd098037ba0 '__int128'
|-TypedefDecl 0x7fd098037f48 <<invalid sloc>> <invalid sloc> implicit __uint128_t 'unsigned __int128'
| `-BuiltinType 0x7fd098037bc0 'unsigned __int128'
|-TypedefDecl 0x7fd098037ff0 <<invalid sloc>> <invalid sloc> implicit SEL 'SEL *'
| `-PointerType 0x7fd098037fa0 'SEL *'
|   `-BuiltinType 0x7fd098037e00 'SEL'
|-TypedefDecl 0x7fd0980380f0 <<invalid sloc>> <invalid sloc> implicit id 'id'
| `-ObjCObjectPointerType 0x7fd098038090 'id'
|   `-ObjCObjectType 0x7fd098038050 'id'
|-TypedefDecl 0x7fd0980381f0 <<invalid sloc>> <invalid sloc> implicit Class 'Class'
| `-ObjCObjectPointerType 0x7fd098038190 'Class'
|   `-ObjCObjectType 0x7fd098038150 'Class'
|-ObjCInterfaceDecl 0x7fd098038248 <<invalid sloc>> <invalid sloc> implicit Protocol
|-TypedefDecl 0x7fd09806ec00 <<invalid sloc>> <invalid sloc> implicit __NSConstantString 'struct __NSConstantString_tag'
| `-RecordType 0x7fd098038390 'struct __NSConstantString_tag'
|   `-Record 0x7fd098038308 '__NSConstantString_tag'
|-TypedefDecl 0x7fd09806ecb0 <<invalid sloc>> <invalid sloc> implicit __builtin_ms_va_list 'char *'
| `-PointerType 0x7fd09806ec60 'char *'
|   `-BuiltinType 0x7fd0980376a0 'char'
|-TypedefDecl 0x7fd09806efb0 <<invalid sloc>> <invalid sloc> implicit __builtin_va_list 'struct __va_list_tag [1]'
| `-ConstantArrayType 0x7fd09806ef50 'struct __va_list_tag [1]' 1
|   `-RecordType 0x7fd09806ed90 'struct __va_list_tag'
|     `-Record 0x7fd09806ed08 '__va_list_tag'
|-ObjCCategoryDecl 0x7fd09806f020 </Users/binbinwang-air/Desktop/callMethod.m:10:1, line:14:2> line:10:12 invalid
|-ObjCInterfaceDecl 0x7fd09806f140 <line:16:1, <invalid sloc>> col:17 implicit ViewController
| `-ObjCImplementation 0x7fd09806f250 'ViewController'
`-ObjCImplementationDecl 0x7fd09806f250 <col:1, line:31:1> line:16:17 ViewController
  |-ObjCInterface 0x7fd09806f140 'ViewController'
  |-ObjCMethodDecl 0x7fd09806f3a8 <line:18:1, line:20:1> line:18:1 - viewDidLoad 'void'
  | |-ImplicitParamDecl 0x7fd09806f860 <<invalid sloc>> <invalid sloc> implicit self 'ViewController *'
  | |-ImplicitParamDecl 0x7fd09806f8c8 <<invalid sloc>> <invalid sloc> implicit _cmd 'SEL':'SEL *'
  | `-CompoundStmt 0x7fd09806f930 <col:21, line:20:1>
  |-ObjCMethodDecl 0x7fd09806f580 <line:22:1, line:25:1> line:22:1 - hello 'void'
  | |-ImplicitParamDecl 0x7fd09806f940 <<invalid sloc>> <invalid sloc> implicit used self 'ViewController *'
  | |-ImplicitParamDecl 0x7fd09806f9a8 <<invalid sloc>> <invalid sloc> implicit _cmd 'SEL':'SEL *'
  | `-CompoundStmt 0x7fd09806fae0 <col:15, line:25:1>
  |   |-ObjCMessageExpr 0x7fd09806fa48 <line:23:5, col:22> 'void' selector=viewDidLoad
  |   | `-ImplicitCastExpr 0x7fd09806fa30 <col:6> 'ViewController *' <LValueToRValue>
  |   |   `-DeclRefExpr 0x7fd09806fa10 <col:6> 'ViewController *' lvalue ImplicitParam 0x7fd09806f940 'self' 'ViewController *'
  |   `-ObjCMessageExpr 0x7fd09806fab0 <line:24:5, col:15> 'void' selector=exit
  |     `-ImplicitCastExpr 0x7fd09806fa98 <col:6> 'ViewController *' <LValueToRValue>
  |       `-DeclRefExpr 0x7fd09806fa78 <col:6> 'ViewController *' lvalue ImplicitParam 0x7fd09806f940 'self' 'ViewController *'
  `-ObjCMethodDecl 0x7fd09806f6f8 <line:27:1, line:29:1> line:27:1 - exit 'void'
    |-ImplicitParamDecl 0x7fd09806fb00 <<invalid sloc>> <invalid sloc> implicit self 'ViewController *'
    |-ImplicitParamDecl 0x7fd09806fb68 <<invalid sloc>> <invalid sloc> implicit _cmd 'SEL':'SEL *'
    `-CompoundStmt 0x7fd09806fbd0 <col:14, line:29:1>
```

如果你之前没有接触过 AST，初一看可能不明觉厉，但相信你从这种结构上也大致能明白这是一种类似Node的结构。

在AST中搜索一下我们关心的 `viewDidLoad`方法，可以发现有两个地方被检索到了（一处是ViewDidLoad方法本身，一处是调用ViewDidLoad方法）：

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu69x6d2qwj60rq0d6dko02.jpg)

其中：

- TranslationUnitDecl ：根节点，表示一个编译单元
- TypedefDecl：表示一个声明
- CompoundStmt：表示陈述
- DeclRefExpr：表示的是表达式
- IntegerLiteral：表示的是字面量，是一种特殊的expr

Clang 里，节点主要分成 Type 类型、Decl 声明、Stmt 陈述这三种，其他的都是这三种的派生。通过扩展这三类节点，就能够将无限的代码形态用有限的形式来表现出来了。

在iOS编译流程上，AST生成后就要交给LLVM做优化和后端编译了，可以说AST是一个和平台无关的中间代码。

### 3. 开发流程介绍

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu6a5ga61hj60ej0cpjrm02.jpg)

#### （1）入口

像iOS工程一样，clang工具的开发，也有一个main入口：

```
//入口main函数
int main(int argc, const char **argv) {
    CommonOptionsParser OptionsParser(argc, argv, ObfOptionCategory);
    ClangTool Tool(OptionsParser.getCompilations(),OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<ObfASTFrontendAction>().get());
}
```

main入口的作用主要是将我们要声明的AST匹配器return出去，比如我们这里要构建的AST匹配器叫`ObfASTFrontendAction`。

#### （2）获取数据源

接着，我们需要获取AST的数据源，获取AST数据源的方式比较简单，只要我们声明一个类继承自`ASTFrontendAction`即可，如下：

```
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
```

#### （3）声明处理人（AST Consumer）

在构建`ASTFrontendAction`时我们构建了 `AST Consumer`，`AST Consumer`中会构建匹配方法：

```
class ObfASTConsumer : public ASTConsumer {
private:
    ClangAutoStatsVisitor Visitor;
public:
    void HandleTranslationUnit(ASTContext &context) {
        TranslationUnitDecl *decl = context.getTranslationUnitDecl();
        Visitor.TraverseTranslationUnitDecl(decl);
    }
};
```

当然了这里可以通过 `RecursiveASTVisitor `构建匹配方法，也可以通过 `AST Matcher`制定匹配规则。

上面是采用了 `ASTConsumer + RecursiveASTVisitor ` 的匹配方式，如果使用 `ASTConsumer  + AST Matcher`的匹配方式，应声明如下：

```
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
```

#### （4）匹配规则

使用 `RecursiveASTVisitor`进行匹配时，重写下面这三个方法就行：

```
bool ObfuscatorVisitor::VisitObjCMessageExpr(ObjCMessageExpr *messageExpr) {
	//遇到了一个消息表达式，例如：[self getName];
    return true;
}


bool ObfuscatorVisitor::VisitObjCImplementationDecl(ObjCImplementationDecl *D) {
 	//遇到了一个类的定义，例如：@implementation ViewController 
	return true;
}


bool ObfuscatorVisitor::VisitObjCInterfaceDecl(ObjCInterfaceDecl *iDecl {
	//遇到了一个类的声明，例如：@interface ViewController : UIViewController
	return true;
}
```

比如我们这里可以写做：

```
class ClangAutoStatsVisitor : public RecursiveASTVisitor<ClangAutoStatsVisitor> {
    
private:
    Rewriter &rewriter;
public:
    explicit ClangAutoStatsVisitor(Rewriter &R) : rewriter{R} {} // 创建方法
    bool VisitObjCMessageExpr(ObjCMessageExpr *messageExpr) {
        cout << "调用的方法：" + messageExpr->getSelector().getAsString() << endl;
        return true;
    }
};
```

还有更便捷的方法，AST Matcher ，可以参考这篇文章：

[基于LLVM的Objective-C代码混淆(二)Clang AST 介绍](http://www.banmalu.top/llvm-02/)

想匹配哪个node，直接搜索就可以了，非常好用：

```
class MatchCallbackHandler : public  MatchFinder::MatchCallback {
public:
    //构造函数
    MatchCallbackHandler(CompilerInstance *aCompilerInstance):compilerInstance(aCompilerInstance)  {}
    virtual void run(const MatchFinder::MatchResult &Result) {
        const ObjCMessageExpr *objCMessageExpr          = Result.Nodes.getNodeAs<ObjCMessageExpr>("objCMessageExpr");

        if (objCMessageExpr) {
            cout << "调用的方法名："<< objCMessageExpr->getSelector().getAsString() << endl;
        }
        
    }
private:
    CompilerInstance *compilerInstance;
};
```

这两种方法我自己都尝试过，个人比较喜欢使用 `AST Matcher`进行AST的解析，完整代码可在github见到：

于是到这里，我们便完成了Clang命令行代码的开发，接下来我们要编译我们的代码，来生成可执行文件进行验证。

## 四、插件化、工具化

因为不是每个人的电脑上都配置了 clang环境，如果我们写好的 clang工具行想发给同事使用，怎么操作呢？

### 1. 重新编译LLVM

在 llvm-project 项目中新建 build 目录，并cd build，将llmv变成成Xcode工程

cmake -G Xcode -DLLVM_ENABLE_PROJECTS=clang ../llvm

这一步是防止我们在开发Clang命令的时候，LLVM工程依赖产生了变更

### 2. Xcode编译

接着我们重新打开 LLVM.xcodeproj ，我们运行Xcode，使用Xcode将我们新建的插件工程编译成命令行工具。

编译完成后，我们可以发现 AddCodePlugin 命令行工具

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu5vf4ccldj60m304cmxk02.jpg)

### 3. 验证脚本

我们在终端执行如下命令：

`/Users/binbinwang-air/Desktop/llvm-project/build/Debug/bin/AddCodePlugin  /Users/binbinwang-air/Desktop/callMethod.m`

可以看到输出：

![](https://tva1.sinaimg.cn/large/008i3skNgy1gu6awoai6yj60li0710tq02.jpg)

我们检测出了有代码主动在调用 `viewDidLoad`，任务完成！

接着聊一下插件化，因为我自己的工作出发点是写一个Clang命令行工具，所以这里没有去生成 Xcode插件 ，另外一个考虑是不同同事Xcode版本不同，插件运行环境不稳定，所以暂时没有制作Xcode插件的需要，如果有的话，我会再补一篇Xcode插件制作的文章。

最后，学习制作Clang命令行工具的过程中，我发现国内在这块的资料比较少，有些博文浅尝辄止，我把我参考过的文章也罗列如下：

- [基于LLVM的Objective-C代码混淆(二)Clang AST 介绍](http://www.banmalu.top/llvm-02/)
- [深入剖析 iOS 编译 Clang / LLVM](https://ming1016.github.io/2017/03/01/deeply-analyse-llvm/)

