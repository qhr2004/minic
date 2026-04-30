flavor 1
srclang "MiniC"
numfuncs 2
entryfunc &main

func &add (var %a i32, var %b i32) i32 {
  # BB entry
  @entry
  return (add i32 (dread i32 %a, dread i32 %b))
}
func &main () i32 {
  var %sum i32
  var %i i32
  var %__inc_old_0 i32
  var %__inc_new_1 i32
  var %x i32

  # BB entry
  @entry
  dassign %sum (constval i32 0)
  # BB for_init
  dassign %i (constval i32 0)
  goto @for_cond_0
  @for_cond_0
  # BB for_cond
  brfalse @for_end_3 (lt i32 i32 (dread i32 %i, constval i32 10))
  goto @for_body_1
  @for_body_1
  # BB for_body
  dassign %sum (add i32 (dread i32 %sum, dread i32 %i))
  call &add (dread i32 %sum, dread i32 %i)
  goto @for_update_2
  @for_update_2
  # BB for_update
  dassign %__inc_old_0 (dread i32 %i)
  dassign %__inc_new_1 (add i32 (dread i32 %i, constval i32 1))
  dassign %i (dread i32 %__inc_new_1)
  goto @for_cond_0
  @for_end_3
  callassigned &add (dread i32 %sum, constval i32 42) { dassign %x 0 }
  return (dread i32 %x)
}
