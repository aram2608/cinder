; ModuleID = 'no-name-lang'
source_filename = "no-name-lang"

define void @add() {
entry:
  %a = alloca i32, align 4
  store i32 3, ptr %a, align 4
  %b = alloca i32, align 4
  store i32 4, ptr %b, align 4
  %a1 = load i32, ptr %a, align 4
  %b2 = load i32, ptr %b, align 4
  %addtmp = add i32 %a1, %b2
  %c = alloca i32, align 4
  store i32 %addtmp, ptr %c, align 4
  %d = alloca float, align 4
  store float 1.000000e+00, ptr %d, align 4
  %f = alloca float, align 4
  store float 2.000000e+00, ptr %f, align 4
  %d3 = load float, ptr %d, align 4
  %f4 = load float, ptr %f, align 4
  %addtmp5 = fadd float %d3, %f4
  %z = alloca float, align 4
  store float %addtmp5, ptr %z, align 4
  ret void
}

define i32 @get() {
entry:
  %a = alloca i32, align 4
  store i32 4, ptr %a, align 4
  %a1 = load i32, ptr %a, align 4
  ret i32 %a1
}

define i32 @main() {
entry:
  call void @add()
  %get = call i32 @get()
  %b = alloca i32, align 4
  store i32 %get, ptr %b, align 4
  ret i32 0
}
