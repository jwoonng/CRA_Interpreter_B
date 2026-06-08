<div align="center">

# 🛠️ CodeFab Interpreter

**직접 만든 언어를 실시간으로 실행하는 인터프리터**

![Language](https://img.shields.io/badge/언어-CodeFab-4a90d9?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?style=flat-square&logo=windows&logoColor=white)

</div>

---

## ▶️ 실행 방법

`factory.exe` 를 터미널에서 실행합니다. 세 가지 모드를 지원합니다.

| 모드 | 명령 | 설명 |
|------|------|------|
| **대화형 (REPL)** | `factory` | 한 줄씩 즉시 실행 |
| **파일 실행** | `factory run <경로>` | 소스 파일 전체를 실행 |
| **디버그** | `factory debug <경로>` | 소스 파일을 단계별로 실행 |

### 대화형 모드 (REPL)

```
> _
```

`>` 프롬프트가 뜨면 코드를 입력하세요. **Enter** 로 즉시 실행됩니다.  
종료하려면 `quit` 또는 `exit` 를 입력하세요.

> 입력한 변수·함수는 종료 전까지 계속 유지됩니다.

중괄호 `{` 로 블록을 열면 `...` 프롬프트로 전환되고,  
`}` 로 닫힐 때 블록 전체가 한꺼번에 실행됩니다.

```
> func add(a, b) {
...     return a + b;
... }
> print add(3, 7);
10
```

> **REPL 선행 검사**: 블록을 입력하는 도중, 선언된 적 없는 변수를 사용하면  
> 해당 줄을 입력하는 즉시 에러가 표시됩니다. 에러가 발생한 줄만 스킵되고  
> 나머지 블록은 유지됩니다.

### 파일 실행 모드

```
factory run hello.cb
```

### 디버그 모드

소스 파일을 불러온 뒤 한 줄씩 실행하며 변수 상태를 확인할 수 있습니다.

```
factory debug hello.cb
```

| 명령 | 설명 |
|------|------|
| `step` / `next` | 다음 문장으로 이동 |
| `continue` | 다음 중단점까지 실행 |
| `break <줄>` | 줄 번호에 중단점 설정 |
| `breakpoints` | 중단점 목록 확인 |
| `remove <줄>` | 중단점 제거 |
| `watch <변수>` | 변수 감시 등록 |
| `unwatch <변수>` | 감시 해제 |
| `watches` | 감시 목록 확인 |
| `inspect <변수>` | 변수 현재 값 출력 |
| `quit` / `exit` | 디버그 종료 |

---

## 📝 기본 사용법

### 변수

```js
var x = 10;
var name = "Alice";
var flag = true;
var empty;        // 값 없이 선언 → nil
```

변수 값은 언제든 바꿀 수 있습니다.

```js
x = x + 1;
```

---

### 출력

```js
print x;
print "안녕하세요, " + name;
print 1 + 2;
```

| 저장된 값 | 출력 |
|:---------:|:----:|
| `10` (정수) | `10` |
| `3.14` | `3.14` |
| `true` / `false` | `true` / `false` |
| 선언만 하고 값 없음 | `nil` |
| `"hello"` | `hello` |

---

### 연산자

```js
// 산술
print 10 + 3;    // 13
print 10 / 3;    // 3.33333...
print 10 * 2;    // 20
print 10 - 4;    // 6
print 10 % 3;    // 1  (나머지, 실수 가능: 2.5 % 1.2 → 0.1)

// 문자열 이어붙이기
print "Hello" + ", " + "World";  // Hello, World

// 비교 (결과는 true / false)
print 5 > 3;     // true
print 3 >= 3;    // true
print 1 != 2;    // true
print 1 == 1;    // true

// 논리
print true and false;  // false
print true or false;   // true
print !true;           // false
```

> **참/거짓 판별**: `false`, `0`, `nil` 만 거짓입니다. 빈 문자열 `""` 도 참입니다.

---

### 조건문

```js
var score = 85;

if (score >= 90) {
    print "A";
} else if (score >= 80) {
    print "B";
} else {
    print "C";
}
```

`else if` 와 `else` 는 생략할 수 있습니다.

---

### 반복문

```js
for (var i = 0; i < 5; i = i + 1) {
    print i;
}
// 출력: 0  1  2  3  4
```

초기화 · 조건 · 증감식은 모두 생략 가능합니다.

```js
var i = 0;
for (; i < 3; i = i + 1) {
    print i;
}
```

---

### 함수

**선언**

```js
func greet(name) {
    print "Hello, " + name;
}
```

**호출**

```js
greet("Alice");   // Hello, Alice
```

**반환값**

```js
func add(a, b) {
    return a + b;
}

var result = add(3, 7);
print result;   // 10
```

`return` 없이 끝나거나 `return;` 이면 `nil` 을 반환합니다.

**재귀**

```js
func fact(n) {
    if (n <= 1) return 1;
    return n * fact(n - 1);
}

print fact(5);   // 120
```

---

### 배열

`array(n)` 으로 n개짜리 배열을 만듭니다. 초기값은 모두 `nil` 입니다.

```js
var arr = array(3);   // [nil, nil, nil]
```

**읽기 / 쓰기**

```js
arr[0] = 10;
arr[1] = 20;
arr[2] = 30;

print arr[0];             // 10
print arr[1] + arr[2];    // 50
```

인덱스에 변수나 식을 쓸 수 있습니다.

```js
var i = 1;
arr[i] = 99;
print arr[i - 1];   // 10
```

**배열을 함수에 넘기기**

배열은 참조로 전달되므로 함수 안에서 수정하면 밖에서도 반영됩니다.

```js
func fill(a, val) {
    a[0] = val;
    a[1] = val;
    a[2] = val;
}

var arr = array(3);
fill(arr, 7);
print arr[0];   // 7
```

---

### 블록과 스코프

중괄호 `{}` 안에서 선언한 변수는 블록 밖에서 보이지 않습니다.

```js
var x = 1;
{
    var x = 2;
    print x;   // 2  (블록 안)
}
print x;       // 1  (블록 밖, 원래 값)
```

---

### 주석

```js
// 이 줄은 실행되지 않습니다
var x = 10;   // 인라인 주석도 가능
```

---

## 💡 예시 모음

### 피보나치 수열

```js
func fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

for (var i = 0; i < 8; i = i + 1) {
    print fib(i);
}
// 0  1  1  2  3  5  8  13
```

### 1부터 10까지 합산

```js
var sum = 0;
for (var i = 1; i <= 10; i = i + 1) {
    sum = sum + i;
}
print sum;   // 55
```

### 문자열 반복

```js
func repeat(s, n) {
    var result = "";
    for (var i = 0; i < n; i = i + 1) {
        result = result + s;
    }
    return result;
}

print repeat("ha", 3);   // hahaha
```

### 최댓값 구하기

```js
func max(a, b) {
    if (a > b) return a;
    return b;
}

print max(7, 13);   // 13
```

### 배열 합산

```js
var arr = array(5);
for (var i = 0; i < 5; i = i + 1) {
    arr[i] = i + 1;
}

var sum = 0;
for (var i = 0; i < 5; i = i + 1) {
    sum = sum + arr[i];
}
print sum;   // 15
```

---

## ✅ 되는 것 · ❌ 안 되는 것

```mermaid
flowchart LR
    subgraph YES ["✅ 되는 것"]
        direction TB
        y1["var 변수 선언·재대입"]
        y2["산술 · 문자열 · 논리 연산 (+ % 포함)"]
        y3["if / else if / else"]
        y4["for 반복문"]
        y5["중첩 블록 · 스코프"]
        y6["func 함수 선언·호출"]
        y7["재귀 함수"]
        y8["return 값 반환"]
        y9["// 줄 주석"]
        y10["array(n) 배열 생성·읽기·쓰기"]
    end

    subgraph NO ["❌ 안 되는 것"]
        direction TB
        n1["함수를 변수에 저장"]
        n2["while 문"]
        n3["break / continue"]
        n4["++ / -- 연산자"]
        n5["+= 복합 대입"]
        n6["/* */ 블록 주석"]
        n7["파일 입출력"]
        n8["클래스 / 구조체"]
    end

    style YES fill:#f0fff4,stroke:#27ae60
    style NO fill:#fff5f5,stroke:#e53e3e
```

---

## ⚠️ 오류 메시지

오류가 나면 다음 메시지를 확인하세요.

<details>
<summary><b>문법 오류 (입력 직후 표시)</b></summary>

| 메시지 | 원인 |
|--------|------|
| `Unexpected character: X` | 지원하지 않는 문자 입력 |
| `Unsupported operator '++'. Use 'x = x + 1'.` | `++` 증감 연산자 사용 |
| `Unsupported operator '--'. Use 'x = x - 1'.` | `--` 감소 연산자 사용 |
| `Unsupported operator '+='. Use 'x = x + value'.` | `+=` 복합 대입 사용 |
| `Unsupported operator '-='. Use 'x = x - value'.` | `-=` 복합 대입 사용 |
| `Unsupported operator '*='. Use 'x = x * value'.` | `*=` 복합 대입 사용 |
| `Unsupported operator '/='. Use 'x = x / value'.` | `/=` 복합 대입 사용 |
| `Unterminated string` | 닫는 `"` 없이 문자열 끝남 |
| `Keywords must be lowercase. Use 'X' instead of 'Y'.` | `Return`, `VAR`, `FOR` 등 대소문자 혼용 keyword 사용 |
| `Type annotations are not supported. Use 'var' instead of 'X'.` | `int x = 1;` 처럼 타입 선언 사용 |
| `Unknown keyword 'X'. Use 'func' to declare functions.` | `Func`, `function`, `def`, `fn` 등 잘못된 함수 키워드 사용 |
| `Unknown keyword 'X'. Use 'var' to declare variables.` | `let`, `const` 등 잘못된 변수 키워드 사용 |
| `Unknown keyword 'X'. Use 'for' for loops.` | `while`, `do` 등 잘못된 반복 키워드 사용 |
| `Unknown keyword 'X'. Use 'print' to output values.` | `println`, `cout` 등 잘못된 출력 키워드 사용 |
| `Unknown keyword 'X'. Use 'else if' for chained conditions.` | `elif`, `elsif` 사용 |
| `Unexpected token 'X'.` | 예상치 못한 토큰 (예: `var x = ;`) |
| `Unexpected end of input.` | 표현식이 완성되기 전에 입력이 끝남 |
| `Unexpected '}': no matching '{' found.` | 짝 없는 `}` 입력 |
| `Expected ')'` / `Expected '('` | 괄호 누락 |
| `Expected ';'` | 세미콜론 누락 |
| `Invalid assignment target.` | 대입 불가 대상에 `=` 사용 |
| `Expected function name.` | `func` 다음에 이름 없음 |

</details>

<details>
<summary><b>선언 오류 (실행 전 검사)</b></summary>

| 메시지 | 원인 |
|--------|------|
| `Variable 'X' is already declared in this block.` | 같은 블록 안에서 같은 이름으로 `var` 두 번 |
| `Variable 'X' is already declared in global scope (first declared at line N).` | 전역 스코프에서 같은 이름으로 `var` 두 번 |
| `Cannot read local variable 'X' in its own initializer.` | `var x = x + 1;` 처럼 자기 자신 참조 |
| `Cannot use 'return' outside of a function.` | 함수 밖에서 `return` 사용 |
| `Duplicate parameter name 'X' in function 'F'.` | 파라미터 이름 중복 |
| `Undefined variable 'X'.` | REPL 블록 입력 중 미선언 변수 참조 (선행 검사) |

</details>

<details>
<summary><b>런타임 오류 (실행 중 발생)</b></summary>

| 메시지 | 원인 |
|--------|------|
| `Undefined variable 'X'.` | 선언하지 않은 변수 사용 |
| `Division by zero.` | `0` 으로 나누기 |
| `Operands must be numbers.` | 숫자가 아닌 값에 산술 연산 |
| `Operands must be two numbers or two strings.` | `+` 에 숫자+문자열 혼용 |
| `'X' is not a function.` | 변수를 함수처럼 호출 |
| `Undefined function 'X'.` | 선언하지 않은 함수 호출 |
| `Expected N arguments but got M.` | 인자 개수 불일치 |
| `Value is not an array.` | 배열이 아닌 변수에 `[ ]` 사용 |
| `Array index must be an integer.` | 인덱스에 정수가 아닌 값 사용 |
| `Array index out of range.` | 배열 크기를 벗어난 인덱스 |
| `Array() expects exactly 1 argument.` | `array()` 인자 개수 오류 |
| `Array size must be a non-negative integer.` | `array` 크기에 음수·소수·문자열 등 사용 |

</details>
