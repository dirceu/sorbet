# typed: true

class A # error: Hint: this "class" token is not closed before the end of the file
  extend T::Sig
  sig {params(x: Integer, y: Integer).void}
  #           ^ error: Unknown argument name `x`
  #                       ^ error: Unknown argument name `y`
  def foo # error: Hint: this "def" token is not closed before the end of the file
    #    ^ completion: foo
    #    ^ apply-completion: [A] item: 0
    # error: unexpected token "end of file"
