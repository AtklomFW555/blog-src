from pygments.lexer import RegexLexer, words, bygroups
from pygments.token import Keyword, Name, String, Number, Punctuation, Operator, Comment, Whitespace, Generic


def kwds(lists):
  if isinstance(lists[0], list):
    lists = [r for l in lists for r in l]
  return words(lists, prefix=r'\b', suffix=r'\b')


def chars(s):
  return words([c for c in s])


class SiberiaLexer(RegexLexer):
  name = 'Siberia'
  aliases = ['siberia', 'sib']
  filenames = ['*.sib', "*.siberia"]

  tokens = {
      'root': [
          (r'(\s|\n|\r)+', Whitespace),
          (r'#=(.|\n)*?\=#|#.*?(\n|$)', Comment),
          (r'\\func> *([a-zA-Z0-9_]+)', bygroups(Name.Function)),
          (r'\\var> *([a-zA-Z0-9_]+)', bygroups(Name)),
          (r'\\op> *([a-zA-Z0-9_]+)', bygroups(Operator)),
          (r'\\num> *([a-zA-Z0-9_]+)', bygroups(Number)),
          (r'\\kwd> *([a-zA-Z0-9_]+)', bygroups(Keyword)),
          (r'\\(\[.*?\])', bygroups(Generic.Output)),
          (r'([0-9][a-zA-Z0-9_]*\.?|\.[0-9])[a-zA-Z0-9_]*', Number),
          (r'"([^"\\]|\\.)*"|\'([^\'\\]|\\.)*\'', String),
          (chars('([{'), Punctuation, 'root'),
          (chars(')]}'), Punctuation, '#pop'),
          (chars(',;'), Punctuation),
          (chars('+-*/%=^&|?:<>!~.'), Operator),
          (r'(# *define|# *undef)( *)([a-zA-Z0-9_]*)', bygroups(Generic.Prompt, Whitespace, Number)),
          (r'# *[a-zA-Z0-9_\-]*', Generic.Prompt),
          (kwds(['true', 'false', 'null']), Number),
          (kwds([
              ['for', 'while', 'do', 'if', 'else', 'elif'],
              ['break', 'continue', 'return'],
          ]), Keyword),
          (r'[A-Z][A-Z0-9_]*_[A-Z0-9_]*', Name.Constant),
          (r'[A-Z][a-zA-Z0-9_]*', Name.Class),
          (r'([a-zA-Z_][a-zA-Z0-9_]*)(\s|\n|\r)*(\()', bygroups(Name.Function, Whitespace, Punctuation)),
          (r'func', Keyword),
      ],
  }
