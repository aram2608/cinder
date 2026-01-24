; ModuleID = 'main'
source_filename = "main"

declare i32 @printf()

define i32 @get(i32 %a, i32 %b) {
entry:
  %addtmp = add i32 %a, %b
  ret i32 %addtmp
}

define i32 @main() {
entry:
  %get = call i32 @get()
  %b = alloca i32, align 4
  store i32 %get, ptr %b, align 4
  ret i32 0
}
