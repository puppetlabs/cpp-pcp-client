#!/usr/bin/env python

'''
Simple script to extract tagged comments from markdown files.
The script will create in the current directory a file named "todo.md"
containing an list of autor/comment pairs, organized by file.
'''

import os
import re
import sys

MARKDOWN_SUFFIX  = ".md"
UNDEFINED_AUTHOR = "n/a"
DEFAULT_OUT_FILE = "./todo.md"


class Comment(object):
    ''' A comment entry '''
    def __init__(self, txt, author = UNDEFINED_AUTHOR):
        self._txt    = txt
        self._author = author

    def __str__(self):
        return "  AUTHOR : %s  -  COMMENT: %s\n" % (self._author, self._txt)


start_regex = re.compile(r'<!--(?P<txt>.*)')
end_regex   = re.compile(r'(?P<txt>.*)-->')
todo_regex  = re.compile(r'(TODO|HERE)\((?P<author>\w+)\):\s?(?P<txt>.*)')


def getCommentGenerator(file_path):
    with open(file_path, 'r') as f_r:
        lines = filter(None, (line.rstrip() for line in f_r))

    if len(lines) == 0:
        return

    found_comment = True
    comment = ''

    for line in lines:
        if not found_comment:
            start_result = start_regex.search(line)

            if start_result:
                comment = start_result.group('txt').strip()

                end_result = end_regex.search(comment)

                if end_result:
                    yield end_result.group('txt').strip()
                else:
                    found_comment = True
        else:
            # we already have a started comment
            end_result = end_regex.search(line)

            if end_result:
                comment += " " + end_result.group('txt').strip()
                found_comment = False
                yield comment
            else:
                comment += " " + line


def getCommentsFromFile(file_path):
    comments = []

    for comment_txt in getCommentGenerator(file_path):
        parsed_comment = todo_regex.search(comment_txt)

        if parsed_comment:
            author  = parsed_comment.group('author')
            comment = parsed_comment.group('txt')
            comments.append(Comment(comment, author))
        else:
            comments.append(Comment(comment_txt))

    return comments


def saveCommentsTo(out_file_path, file_comments):
    lines = []

    for file_name, comments in file_comments.iteritems():
        if len(comments):
            lines.append("\n#### %s\n" % file_name)
            lines.extend([str(c) for c in comments])

    with open(out_file_path, 'w') as f_w:
        for line in lines:
            f_w.write(line + '\n')


def processFilesIn(root_path):
    file_comments = {}

    for dir_path, _, files in os.walk(root_path):
        for file_name in files:
            if file_name.endswith(MARKDOWN_SUFFIX):
                file_path = os.path.join(dir_path, file_name)
                comments = getCommentsFromFile(file_path)
                file_comments[file_name[:-3]] = comments

    saveCommentsTo(os.path.abspath(DEFAULT_OUT_FILE), file_comments)


def main(argv = None):
    argv = sys.argv

    if argv is not None and len(argv) == 2:

        root_path = os.path.abspath(argv[1])

        if not os.path.isdir(root_path):
            print("%s is not a valid directory path" % root_path)
            return 1
        else:
            processFilesIn(root_path)

    else:
        print("You must specify only the root directory path")
        return 1


if __name__ == '__main__':
    sys.exit(main())
