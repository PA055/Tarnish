import sys
import os


def defineVisitor(f, baseName: str, types: dict):
    f.write("""
template <class R>
class Visitor {
public:
    virtual ~Visitor() = default;
""")
    for name, fields in types.items():
        f.write(f"\tvirtual R visit{name}{baseName}(std::shared_ptr<{name}<R>> {baseName.lower()}) = 0;\n")
    f.write("};\n")


def declareTypes(f, baseName: str, types: dict):
    for name in types.keys():
        f.write(f"\ntemplate<class R>\nclass {name};\n\n")


def defineType(f, baseName: str, className: str, fields: str):
    f.write(f"""
template <class R>
class {className} :
        public {baseName}<R>,
        public std::enable_shared_from_this<{className}<R>> {{
public:
    {className}({fields}) :
""")
    for field in fields.split(', ')[:-1]:
        fieldType, fieldName = field.strip().split(' ')
        f.write(f"\t\t\t{fieldName}({fieldName}),\n")
    f.write(f"\t\t\t{fields.split(', ')[-1].split(' ')[1]}({fields.split(', ')[-1].split(' ')[1]}) {{}}\n\n")
    f.write(f"""\tR accept(std::shared_ptr<Visitor<R>> visitor) override {{
        return visitor->visit{className}{baseName}(this->shared_from_this());
    }}
""")
    for field in fields.split(', '):
        fieldType, fieldName = field.strip().split(' ')
        f.write(f"\t{fieldType} {fieldName};\n")
    f.write("};\n")


def defineAst(outputDir: str, baseName: str, types: dict):
    path = os.path.join(outputDir, baseName.lower() + '.hpp')
    f = open(path, 'w')
    f.write("""#pragma once

#include <memory>

#include "token.hpp"
""")
    declareTypes(f, baseName, types)
    defineVisitor(f, baseName, types)
    f.write(f"""
template <class R>
class {baseName} {{
public:
    virtual ~{baseName}() = default;
    virtual R accept(std::shared_ptr<Visitor<R>> visitor) = 0;
}};
""")
    for name, fields in types.items():
        defineType(f, baseName, name, fields)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python astGen.py <output_directory>")
        exit(64)

    outputDir = sys.argv[1]
    defineAst(outputDir, "Expr", {
        "Binary": "std::shared_ptr<Expr<R>> left, Token operation, std::shared_ptr<Expr<R>> right",
        "Grouping": "std::shared_ptr<Expr<R>> expression",
        "Literal": "Object value",
        "Unary": "Token operation, std::shared_ptr<Expr<R>> right",
    })
