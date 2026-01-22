; ModuleID = 'no-name-lang'
source_filename = "no-name-lang"

declare i32 @printf()

define i32 @get(i32 %a, i32 %b) {
entry:
  %a1 = alloca i32, align 4
  store i32 1, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 2, ptr %b2, align 4
  %a3 = load i32, ptr %a1, align 4
  %b4 = load i32, ptr %b2, align 4
  %addtmp = add i32 %a3, %b4
  ret i32 %addtmp
}

define i32 @main() {
entry:
  %get = call i32 @get()
  %b = alloca i32, align 4
  store i32 %get, ptr %b, align 4
  ret i32 0
}
