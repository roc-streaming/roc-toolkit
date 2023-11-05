#! /usr/bin/env python3

import argparse
import os
import string
import subprocess
import sys
import textwrap
import xml.etree.ElementTree as ElementTree
from dataclasses import dataclass

CONFIG_FILE_PATH = "../build/docs/public_api/xml/config_8h.xml"

ROC_JAVA_BASE_PATH = "../../roc-java"
ROC_GO_BASE_PATH = "../../roc-go"

ROC_JAVA_PACKAGE = "org.rocstreaming.roctoolkit"

ODD_PREFIXES = {'roc_clock_source': 'ROC_CLOCK_', 'roc_protocol': 'ROC_PROTO_'}


def to_camel_case(name):
    return ''.join(x.capitalize() for x in name.split('_'))


@dataclass
class DocItem:
    type: string
    text: string = None
    values: list[list['DocItem']] = None


@dataclass
class DocComment:
    items: list[list[DocItem]]


@dataclass
class EnumValue:
    name: string
    value: string
    doc: DocComment


@dataclass
class EnumDefinition:
    name: string
    values: list[EnumValue]
    doc: DocComment


class EnumGenerator:
    def generate_enum(self, enum_definition: EnumDefinition, autogen_comment: list[string]):
        raise NotImplementedError


class JavaEnumGenerator(EnumGenerator):

    def __init__(self, base_path: string, name_prefixes: dict[str, str]):
        self.base_path = base_path
        self.name_prefixes: dict[str, str] = name_prefixes

    def generate_enum(self, enum_definition: EnumDefinition, autogen_comment: list[string]):
        java_name = self.get_java_enum_name(enum_definition.name)
        enum_values = enum_definition.values

        enum_file_path = (self.base_path + "/src/main/java/"
                          + ROC_JAVA_PACKAGE.replace(".", "/") + "/" + java_name + ".java")
        enum_file = open(enum_file_path, "w")

        for line in autogen_comment:
            enum_file.write("// " + line + "\n")
        enum_file.write("\n")
        enum_file.write(f"package {ROC_JAVA_PACKAGE};\n\n")
        enum_file.write(self.format_javadoc(enum_definition.doc, 0))
        enum_file.write("public enum " + java_name + " {\n")

        for enum_value in enum_values:
            java_enum_value = self.get_java_enum_value(enum_definition.name, enum_value.name)
            enum_file.write("\n")
            enum_file.write(self.format_javadoc(enum_value.doc, 4))
            enum_file.write("    " + java_enum_value + "(" + enum_value.value + "),\n")

        enum_file.write("    ;\n\n")

        enum_file.write("    final int value;\n\n")

        enum_file.write("    " + java_name + "(int value) {\n")
        enum_file.write("        this.value = value;\n")
        enum_file.write("    }\n")
        enum_file.write("}\n")

        enum_file.close()

    def get_java_enum_value(self, enum_name, enum_value_name):
        prefix = self.name_prefixes.get(enum_name)
        return enum_value_name.removeprefix(prefix)

    def get_java_enum_name(self, roc_enum_name):
        return to_camel_case(roc_enum_name.removeprefix('roc_'))

    def get_java_link(self, roc_enum_value_name):
        for roc_enum_name in self.name_prefixes:
            prefix = self.name_prefixes.get(roc_enum_name)
            if roc_enum_value_name.startswith(prefix):
                java_type = self.get_java_enum_name(roc_enum_name)
                java_enum = self.get_java_enum_value(roc_enum_name, roc_enum_value_name)
                return f"{java_type}#{java_enum}"

    def format_javadoc(self, doc: DocComment, indent_size: int):
        indent = " " * indent_size
        indent_line = indent + " * "

        doc_string = indent + "/**\n"

        for i, items in enumerate(doc.items):
            if i != 0:
                doc_string += indent + " * <p>\n"

            text = self.doc_item_to_string(items)
            for t in text.split("\n"):
                lines = textwrap.wrap(t, width=80,
                                      break_on_hyphens=False,
                                      initial_indent=indent_line,
                                      subsequent_indent=indent_line)
                for line in lines:
                    doc_string += line + "\n"

        doc_string += indent + " */\n"
        return doc_string

    def doc_item_to_string(self, items: list[DocItem]):
        result = []
        for item in items:
            t = item.type
            if t == "text":
                result.append(item.text)
            elif t == "ref" or t == "code":
                result.append(self.ref_to_string(item.text))
            elif t == "list":
                ul = "<ul>\n"
                for li in item.values:
                    ul += f"<li>{self.doc_item_to_string(li)}</li>\n"
                ul += "</ul>\n"
                result.append(ul)
            else:
                print(f"unknown doc item type = {t}, consider adding it to doc_item_to_string")
        return ' '.join(result).replace(" ,", ",").replace(" .", ".")

    def ref_to_string(self, ref_value):
        """
        :param ref_value: enum_value or enum_type, e.g. roc_endpoint or ROC_INTERFACE_CONSOLIDATED
        :return: java link javadoc
        """
        if ref_value.startswith("roc_"):
            java_name = self.get_java_enum_name(ref_value)
            return "{@link " + java_name + "}"

        link = self.get_java_link(ref_value)
        return "{@link " + link + "}"


class GoEnumGenerator(EnumGenerator):

    def __init__(self, base_path: string, name_prefixes: dict[str, str]):
        self.base_path = base_path
        self.name_prefixes: dict[str, str] = name_prefixes

    def generate_enum(self, enum_definition: EnumDefinition, autogen_comment: list[string]):
        go_name = enum_definition.name.removeprefix('roc_')
        enum_values = enum_definition.values

        enum_file_path = self.base_path + "/roc/" + go_name + ".go"
        enum_file = open(enum_file_path, "w")

        go_type_name = to_camel_case(go_name)
        for line in autogen_comment:
            enum_file.write("// " + line + "\n")
        enum_file.write("\n")
        enum_file.write("package roc\n\n")
        enum_file.write(self.format_comment(enum_definition.doc, ""))
        enum_file.write("//\n")
        roc_prefix = self.name_prefixes[enum_definition.name]
        go_prefix = to_camel_case(roc_prefix.lower().removeprefix('roc_').removesuffix('_'))
        enum_file.write(
            f"//go:generate stringer -type {go_type_name} -trimprefix {go_prefix} -output {go_name}_string.go\n")
        enum_file.write(f"type {go_type_name} int\n\n")

        enum_file.write("const (\n")

        for i, enum_value in enumerate(enum_values):
            go_enum_value = to_camel_case(enum_value.name.lower().removeprefix('roc_'))

            if i != 0:
                enum_file.write("\n")
            enum_file.write(self.format_comment(enum_value.doc, "\t"))
            enum_file.write(f"\t{go_enum_value} {go_type_name} = {enum_value.value}\n")

        enum_file.write(")\n")

        enum_file.close()

    def format_comment(self, doc: DocComment, indent: string):
        indent_line = indent + "// "
        doc_string = ""

        for i, items in enumerate(doc.items):
            if i != 0:
                doc_string += indent_line.rstrip() + "\n"

            text = self.doc_item_to_string(items)
            for t in text.split("\n"):
                lines = textwrap.wrap(t, width=80,
                                      break_on_hyphens=False,
                                      initial_indent=indent_line,
                                      subsequent_indent=indent_line)
                for line in lines:
                    doc_string += line + "\n"

        return doc_string

    def doc_item_to_string(self, doc_item: list[DocItem]):
        result = []
        for item in doc_item:
            t = item.type
            if t == "text":
                result.append(item.text)
            elif t == "ref" or t == "code":
                result.append(self.ref_to_string(item.text))
            elif t == "list":
                ul = "\n"
                for li in item.values:
                    ul += f" - {self.doc_item_to_string(li)}\n"
                ul += "\n"
                result.append(ul)
            else:
                print(f"unknown doc item type = {t}, consider adding it to doc_item_to_string")
        return ' '.join(result).replace(" ,", ",").replace(" .", ".")

    def ref_to_string(self, ref_value):
        """
        :param ref_value: enum_value or enum_type, e.g. roc_endpoint or ROC_INTERFACE_CONSOLIDATED
        :return: go value
        """
        if ref_value.startswith("roc_"):
            return to_camel_case(ref_value.removeprefix('roc_'))

        return to_camel_case(ref_value.lower().removeprefix('roc_'))


def parse_config_xml():
    try:
        tree = ElementTree.parse(CONFIG_FILE_PATH)
        return tree.getroot()
    except FileNotFoundError:
        print(f"File not found: {CONFIG_FILE_PATH}", file=sys.stderr)
        exit(1)
    except ElementTree.ParseError:
        print(f"Error parsing XML file: {CONFIG_FILE_PATH}", file=sys.stderr)
        exit(1)


def get_enums(root) -> list[EnumDefinition]:
    enum_definitions = []
    enum_memberdefs = root.findall('.//sectiondef[@kind="enum"]/memberdef[@kind="enum"]')
    for member_def in enum_memberdefs:
        name = member_def.find('name').text
        print(f"found enum in docs: {name}")
        doc = parse_doc(member_def)
        values = []

        for enum_value in member_def.findall('enumvalue'):
            enum_name = enum_value.find('name').text
            value = enum_value.find('initializer').text.removeprefix('= ')
            enum_value_doc = parse_doc(enum_value)
            values.append(EnumValue(enum_name, value, enum_value_doc))

        enum_definitions.append(EnumDefinition(name, values, doc))

    return enum_definitions


def parse_doc(elem) -> DocComment:
    items = []
    brief = elem.find('briefdescription/para')
    items.append(parse_doc_elem(brief))

    for para in elem.findall('detaileddescription/para'):
        items.append(parse_doc_elem(para))

    return DocComment(items)


def strip_text(text):
    if text:
        strip = text.strip()
        if strip:
            return strip
    return None


def parse_doc_elem(elem: ElementTree.Element) -> list[DocItem]:
    items = []
    tag = elem.tag
    parse_children = True
    text = strip_text(elem.text)
    if tag == "para":
        if text:
            items.append(DocItem("text", text))
    elif tag == "ref":
        if text:
            items.append(DocItem("ref", text))
    elif tag == "computeroutput":
        if text:
            items.append(DocItem("code", text))
    elif tag == "itemizedlist":
        values = []
        for li in elem.findall("listitem"):
            li_values = []
            for e in li:
                li_values.extend(parse_doc_elem(e))
            values.append(li_values)
        items.append(DocItem("list", values=values))
        parse_children = False
    else:
        print(f"unknown tag = {tag}, consider adding it to parse_doc_elem")
    if parse_children:
        for item in elem:
            items.extend(parse_doc_elem(item))
            if item.tail:
                strip = item.tail.strip()
                if strip:
                    items.append(DocItem("text", text=strip))
    return items


def get_name_prefixes(enum_definitions):
    name_prefixes = {}
    for enum_definition in enum_definitions:
        name = enum_definition.name
        prefix = ODD_PREFIXES.get(name, name.upper() + "_")
        name_prefixes[name] = prefix
    return name_prefixes


def generate_enums(generator_construct, output_dir, name_prefixes, enum_definitions: list[EnumDefinition]):
    if not os.path.isdir(output_dir):
        print(f"Directory does not exist: {output_dir}. Can't generate enums {generator_construct.__name__}",
              file=sys.stderr)
        exit(1)
    git_tag = subprocess.check_output(['git', 'describe', '--tags']).decode('ascii').strip()
    git_commit = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()
    autogen_comment = [f"DO NOT EDIT! Code generated by generate_enums script from roc-toolkit",
                       f"roc-toolkit git tag: {git_tag}, commit: {git_commit}"]
    generator = generator_construct(output_dir, name_prefixes)
    for enum_definition in enum_definitions:
        generator.generate_enum(enum_definition, autogen_comment)


def main():
    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    parser = argparse.ArgumentParser(description='Generate enums')

    parser.add_argument('--type', choices=['all', 'java', 'go'], help='Type of enum generation', required=True)
    parser.add_argument('--go_output_dir',
                        default=ROC_GO_BASE_PATH, help=f"Go output directory (default: {ROC_GO_BASE_PATH})")
    parser.add_argument('--java_output_dir',
                        default=ROC_JAVA_BASE_PATH, help=f"Java output directory (default: {ROC_JAVA_BASE_PATH})")

    args = parser.parse_args()

    xml_doc = parse_config_xml()
    enum_definitions = get_enums(xml_doc)
    name_prefixes = get_name_prefixes(enum_definitions)

    if args.type == "all" or args.type == "java":
        generate_enums(JavaEnumGenerator, args.java_output_dir, name_prefixes, enum_definitions)

    if args.type == "all" or args.type == "go":
        generate_enums(GoEnumGenerator, args.go_output_dir, name_prefixes, enum_definitions)


if __name__ == '__main__':
    main()
