; ModuleID = 'main'
source_filename = "main"

define i32 @get(i32 %a, i32 %b) {
entry:
  %addtmp = add i32 %a, %b
  ret i32 %addtmp
}

define i32 @main() {
entry:
  %get = call i32 @get(i32 1, i32 4)
  %b = alloca i32, align 4
  store i32 %get, ptr %b, align 4
  %flag = alloca i1, align 1
  store i1 false, ptr %flag, align 1
  %b1 = load i32, ptr %b, align 4
  store i32 4, ptr %b, align 4
  %b2 = load i32, ptr %b, align 4
  %subtmp = sub i32 %b2, 1
  store i32 %subtmp, ptr %b, align 4
  %get3 = call i32 @get(i32 1, i32 6)
  %b4 = load i32, ptr %b, align 4
  store i32 %get3, ptr %b, align 4
  ret i32 0
}
