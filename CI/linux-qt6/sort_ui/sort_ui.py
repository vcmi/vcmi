#!/usr/bin/env python3
# encoding: utf-8

from lxml import etree

# Inspired from here: https://stackoverflow.com/a/46128043

def sort_children(node, klass, key):
    """ Sort children along tag and given attribute.
    if attr is None, sort along all attributes"""
    if not isinstance(node.tag, str):  # PYTHON 2: use basestring instead
        # not a TAG, it is comment or DATA
        # no need to sort
        return

    if node.get("class") == klass:
        # sort child along attr
        node[:] = sorted(node, key=key)

    # and recurse
    for child in node:
        sort_children(child, klass, key)


def row_col_key(node):
    """Return the sorting key of an xml node
    using tag and attributes
    """
    row = node.get("row") if node.get("row") else -1
    col = node.get("column") if node.get("column") else -1

    return "{:04d}{:04d}".format(int(row), int(col))


def remove_attr(node, attr, value):
    if not isinstance(node.tag, str):  # PYTHON 2: use basestring instead
        # not a TAG, it is comment or DATA
        # no need to sort
        return

    if node.get(attr) == value:
        node.attrib.pop(attr)

    for child in node:
        remove_attr(child, attr, value)



def process(infile, outfile, sort_qgridlayout=True, remove_stdset=True, remove_native=True):

    tree = etree.parse(infile)
    root = tree.getroot()

    if sort_qgridlayout:
        sort_children(root, "QGridLayout", row_col_key)
    if remove_stdset:
        remove_attr(root, "stdset", "0")
    if remove_native:
        remove_attr(root, "native", "true")

    tree.write(outfile,encoding="UTF-8",xml_declaration=True,pretty_print=True)


import argparse

parser = argparse.ArgumentParser(prog="sort_ui",
                                 description="Qt UI file post processor")

parser.add_argument("filenames",
                    metavar="filename",
                    type=str,
                    nargs = "+",
                    help="The path to the unsorted input file (*.ui). If no outfile is given, the changes will be applied in-place.")

parser.add_argument("-o",
                    "--outfile",
                    metavar="outfile",
                    type=str,
                    help="The path to the sorted output file.")

parser.add_argument("--no-sort-qgridlayout",
                    action="store_false",
                    dest="sort_qgridlayout",
                    help="Sort children of QGridLayout by row - column (default: true)")

parser.add_argument("--no-remove-stdset",
                    action="store_false",
                    dest="remove_stdset",
                    help="Do not remove any stdset=\"0\" attribute (default: false)")

parser.add_argument("--no-remove-native",
                    action="store_false",
                    dest="remove_native",
                    help="Do not remove any native=\"true\" attribute (default: false)")

parser.add_argument("--version",
                    action="version",
                    version="0.0.2")

parser.set_defaults(sort_qgridlayout=True, remove_stdset=True, remove_native=True)

# Execute the parse_args() method
args = parser.parse_args()


if args.outfile is not None:
    if len(args.filenames) == 1:
        filename_out = args.filenames[1]
        process(filename, filename_out, args.sort_qgridlayout, args.remove_stdset, args.remove_native)
        exit(0)
    else:
        print("-o, --output option can only be used with a single input file")
        exit(1)

for filename in args.filenames:
    process(filename, filename, args.sort_qgridlayout, args.remove_stdset, args.remove_native)
