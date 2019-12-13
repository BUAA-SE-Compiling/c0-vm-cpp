#include "./vm.h"
#include "./file.h"
#include "./exception.h"
#include "./util/print.hpp"
#include "argparse.hpp"

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <exception>

void disassemble_binary(std::ifstream* in, std::ostream* out) {
    try {
        File f = File::parse_file_binary(*in);
        f.output_text(*out);
    }
    catch (const InvalidFile& e) {
        println(std::cerr, e.what());
    }
    catch (const std::exception& e) {
        println(std::cerr, e.what());
    }
}

void assemble_text(std::ifstream* in, std::ofstream* out) {
    try {
        File f = File::parse_file_text(*in);
        // f.output_text(std::cout);
        f.output_binary(*out);
    }
    catch (const InvalidFile& e) {
        println(std::cerr, e.what());
    }
    catch (const std::exception& e) {
        println(std::cerr, e.what());
    }
}

void execute(std::ifstream* in, std::ostream* out) {
    try {
        File f = File::parse_file_binary(*in);
        auto avm = std::move(vm::VM::make_vm(f));
        avm->start();
    }
    catch (const InvalidFile& e) {
        println(std::cerr, e.what());
    }
    catch (const std::exception& e) {
        println(std::cerr, e.what());
    }
}

int main (int argc, char** argv) {
    argparse::ArgumentParser program("vm");
	program.add_argument("input")
        .required()
		.help("speicify the file to be assembled/executed.");
	program.add_argument("-d")
		.default_value(false)
		.implicit_value(true)
		.help("disassemble the binary input file.");
	program.add_argument("-a")
		.default_value(false)
		.implicit_value(true)
		.help("assemble the text input file.");
    program.add_argument("-r")
		.default_value(false)
		.implicit_value(true)
		.help("interpret the binary input file.");
    program.add_argument("output")
		.default_value(std::string("-"))
        .required()
		.help("specify the output file.");

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		std::cout << program;
		exit(2);
	}

	auto input_file = program.get<std::string>("input");
	auto output_file = program.get<std::string>("output");
	std::ifstream* input;
	std::ostream* output;
	std::ifstream inf;
	std::ofstream outf;
    //std::unique_ptr<vm::VM> vm;

    if (program["-d"] == true) {
        if (program["-a"] == true || program["-r"] == true) {
            exit(2);
        }
        
        inf.open(input_file, std::ios::binary | std::ios::in);
        if (!inf) {
            exit(2);
        }
        input = &inf;

        if (output_file != "-") {
            if (input_file == output_file) {
                output_file += ".out";
            }
            outf.open(output_file, std::ios::out | std::ios::trunc);
            if (!outf) {
                inf.close();
                exit(2);
            }
            output = &outf;
        }
        else {
            output = &std::cout;
        }
        disassemble_binary(input, output);
    }
    else if (program["-a"] == true) {
        if (program["-r"] == true) {
            exit(2);
        }
        if (program["-d"] == true || program["-r"] == true) {
            exit(2);
        }
        
        inf.open(input_file, std::ios::in);
        if (!inf) {
            exit(2);
        }
        input = &inf;

        if (output_file == "-" || input_file == output_file) {
            output_file = input_file + ".out";
        }
        outf.open(output_file, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!outf) {
            inf.close();
            exit(2);
        }
        output = &outf;
        assemble_text(input, dynamic_cast<std::ofstream*>(output));
    }
    else if (program["-r"] == true) {
        inf.open(input_file, std::ios::binary | std::ios::in);
        if (!inf) {
            exit(2);
        }
        input = &inf;

        if (output_file != "-") {
            if (input_file == output_file) {
                output_file += ".out";
            }
            outf.open(output_file, std::ios::out | std::ios::trunc);
            if (!outf) {
                inf.close();
                exit(2);
            }
            output = &outf;
        }
        else {
            output = &std::cout;
        }

        execute(input, output);
    }
    else {
        exit(2);
    }
    inf.close();
    outf.close();
    return 0;
}