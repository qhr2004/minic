flavor 1
srclang "MiniC"
numfuncs 1
entryfunc &main

func &main () i32 {
  var %i i32
  var %sum i32

  # BB entry
  @entry
  dassign %i (constval i32 3)
  dassign %sum (constval i32 0)
  # BB if_cond
  brfalse @if_else_0 (gt i32 i32 (dread i32 %i, constval i32 1))
  # BB if_then
  dassign %sum (add i32 (dread i32 %sum, dread i32 %i))
  goto @if_end_1
  @if_else_0
  # BB if_else
  dassign %sum (sub i32 (dread i32 %sum, constval i32 1))
  @if_end_1
  goto @while_cond_2
  @while_cond_2
  # BB while_cond
  brfalse @while_end_4 (dread i32 %i)
  goto @while_body_3
  @while_body_3
  # BB while_body
  dassign %sum (add i32 (dread i32 %sum, dread i32 %i))
  dassign %i (sub i32 (dread i32 %i, constval i32 1))
  goto @while_cond_2
  @while_end_4
  return (dread i32 %sum)
}
