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
  var %j i32
  var %__inc_old_0 i32
  var %__inc_new_1 i32
  var %__inc_old_2 i32
  var %__inc_new_3 i32
  var %x i32
  var %y i32
  var %__inc_old_4 i32
  var %__inc_new_5 i32
  var %__callret_6 i32
  var %z i32
  var %__inc_old_7 i32
  var %__inc_new_8 i32

  # BB entry
  @entry
  dassign %sum (constval i32 0)
  dassign %i (constval i32 0)
  # BB for_init
  dassign %i (constval i32 0)
  goto @for_cond_0
  @for_cond_0
  # BB for_cond
  brfalse @for_end_3 (lt i32 i32 (dread i32 %i, constval i32 5))
  goto @for_body_1
  @for_body_1
  # BB for_body
  dassign %sum (add i32 (dread i32 %sum, dread i32 %i))
  call &add (dread i32 %sum, dread i32 %i)
  # BB for_init
  dassign %j (constval i32 0)
  goto @for_cond_4
  @for_cond_4
  # BB for_cond
  brfalse @for_end_7 (lt i32 i32 (dread i32 %j, constval i32 3))
  goto @for_body_5
  @for_body_5
  # BB for_body
  dassign %sum (add i32 (dread i32 %sum, dread i32 %j))
  call &add (dread i32 %sum, dread i32 %j)
  goto @for_update_6
  @for_update_6
  # BB for_update
  dassign %__inc_old_0 (dread i32 %j)
  dassign %__inc_new_1 (add i32 (dread i32 %j, constval i32 1))
  dassign %j (dread i32 %__inc_new_1)
  goto @for_cond_4
  @for_end_7
  goto @for_update_2
  @for_update_2
  # BB for_update
  dassign %__inc_old_2 (dread i32 %i)
  dassign %__inc_new_3 (add i32 (dread i32 %i, constval i32 1))
  dassign %i (dread i32 %__inc_new_3)
  goto @for_cond_0
  @for_end_3
  callassigned &add (dread i32 %sum, constval i32 42) { dassign %x 0 }
  dassign %__inc_old_4 (dread i32 %i)
  dassign %__inc_new_5 (add i32 (dread i32 %i, constval i32 1))
  dassign %i (dread i32 %__inc_new_5)
  callassigned &add (dread i32 %x, constval i32 2) { dassign %__callret_6 0 }
  dassign %y (add i32 (dread i32 %__inc_old_4, dread i32 %__callret_6))
  dassign %__inc_old_7 (dread i32 %i)
  dassign %__inc_new_8 (add i32 (dread i32 %i, constval i32 1))
  dassign %i (dread i32 %__inc_new_8)
  callassigned &add (dread i32 %y, dread i32 %__inc_old_7) { dassign %z 0 }
  return (add i32 (add i32 (dread i32 %z, dread i32 %sum), dread i32 %i))
}
