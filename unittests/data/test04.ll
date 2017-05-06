define void @test() {
entry:
  %p = alloca i32*, align 8
  %call = call noalias i8* @malloc(i64 4)
  %0 = bitcast i8* %call to i32*
  store i32* %0, i32** %p, align 8
  ret void
}

declare noalias i8* @malloc(i64)

