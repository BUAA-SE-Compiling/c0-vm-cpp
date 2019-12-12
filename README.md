# c0-vm-cpp
a messy cpp implementation of c0 VM



本项目包括 c0 的汇编器、反汇编器、虚拟机

在 Windows 10 家庭版 64bit 、 CentOS 7.3 64 bit 以及 dokcer 镜像 中经过简单的测试通过，测试样例见 `sample/` 目录（该目录下的文件在最终评测时也会使用）。

**~~大概率有bug~~**。如果发现问题，请附上对应的文件，以便尽快修复。

在根目录执行如下操作，可在`build/src/`目录下找到名为 `c0-vm-cpp` 的编译产物（使用C++17编写，请确保你的编译器支持）：

```
mkdir build
cd build
cmake ..
make # mingw32-make
```

使用方式

```
Usage: vm [options] input output

Positional arguments:
input           speicify the file to be assembled/executed.[Required]
output          specify the output file.[Required]

Optional arguments:
-h --help       show this help message and exit
-d              disassemble the binary input file.
-a              assemble the text input file.
-r              interpret the binary input file.
```

每次使用只能带有一种选项参数，且必须有`input`参数：

- `-h`，显示帮助
- `-a input output`，输入文本汇编文件`input`，将其汇编为二进制的文件`output`；不指定`output`则会默认输出到`input.out`
- `-d input output`，输入二进制文件`input`，输出为文本汇编文件`output`；不指定`output`则默认是标准输出流
- `-r input`，输入二进制文件`input`并使用虚拟机运行，虚拟机使用标准输入流和标准输出流，与参数无关







