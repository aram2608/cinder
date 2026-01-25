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
  %b1 = load i32, ptr %b, align 4
  store i32 4, ptr %b, align 4
  %get2 = call i32 @get(i32 1, i32 6)
  %b3 = load i32, ptr %b, align 4
  store i32 %get2, ptr %b, align 4
  ret i32 0
}
