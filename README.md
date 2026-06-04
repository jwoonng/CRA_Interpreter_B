# CodeFab Interpreter

Code Review Agent (C++ 과정)

**팀명**: Build Clean
- 팀장: 우상욱 님
- 팀원: 최종원 님, 이수련 님, 임지웅 님 
- 팀명 의미 : 동작하는 코드에서 끝나지 않고, 읽기 쉽고 유지보수하기 좋은 클린 코드를 만들자.

### Ground Rule
- **17시 퇴근하기**
- 개발 전략
  - 전체 directory 구조 먼저 생성
  - 각 feature별 Branch를 만들어 개발 진행
  - 개발 branch naming: feature/assem.../sub 기능
  - 함수, 변수 명 snake 형식으로 개발
  - 필요한 클래스 생성 및 호출 등은 header에서 진행
  - 프로젝트 개발 시 대부분 소스에서 필요한 라이브러리는 common에서 개발
  - 가능한 모듈을 동시을 동시에 개발
- PR 전략 
  - git hub 전담 관리자: pr + merge 시 comment 확인 및 commit 점이 예쁘게 쌓일수 있게 각 comment의 퀄리티 확인 
  - PR template을 활용
  - Approve는 1명 이상
  - Build Check(Error 제거 확인) 후 PR
  - Master merge 전 개인 Branch에 Master Merge 후 Push
  - Merge PR 전 공유 후 순차적으로 PR 및 Merge
  - 16시 30분 이후 PR금지
  - 개발 branch는 pr 후 삭제한다.
- Comment 말머리  
   - [Must] 반드시 수정 필요
   - [Recommend] 권장 사항
   - [Question] 궁금한 점
   - [Nit] 사소한 제안
   - [Praise] 좋았던 점

## 담당

| 역할 | 담당 모듈 | 경로 |
|------|----------|------|
| 임지웅 님 | Tokenizer (어휘 분석) | `src/assembler/Tokenizer` |
| 최종원 님 | Parser (구문 분석) | `src/assembler/Parser` |
| 이수련 님 | Checker (의미 분석) | `src/checker/Checker` |
| 우상욱 님 | Executor (실행) | `src/executor/Executor` |
| 최종원 님 | Shell (REPL 통합) | `src/shell/Shell` |

---

## 파이프라인 구조

```
Shell       REPL — 줄 단위로 아래 파이프라인 반복 실행
   ↓
소스 코드 (한 줄)
   ↓
Tokenizer   소스 문자열 → Token 목록
   ↓
Parser      Token 목록 → AST (Stmt 목록)
   ↓
Checker     AST 의미 검증 (미선언 변수 등)
   ↓
Executor    AST 실행 → 출력
```

---

## 지원 문법

```
// 변수 선언 및 대입
var x = 10;
x = x + 1;

// 출력
print x;

// 조건문
if (x > 5) print "big"; else print "small";

// 반복문
for (var i = 0; i < 3; i = i + 1) { print i; }

// 블록 (스코프)
{ var y = "inner"; print y; }

// 연산자
// 산술: + - * /
// 비교: == != > >= < <=
// 논리: and or !
// 문자열 연결: "hello" + " world"

// 줄 주석
// 이 줄은 무시됩니다
var x = 10; // 인라인 주석도 가능
```

---

## 빌드 및 실행

**환경**: Visual Studio 2022, Windows, C++20

진입점은 빌드 구성에 따라 분리된다. `vcxproj`의 `ExcludedFromBuild`로 구성별로 전환된다.

#### Debug 빌드 — 테스트 실행 (`test_main.cpp`)

```
msbuild Project17.vcxproj /p:Configuration=Debug /p:Platform=x64

# 전체 테스트 실행
.\x64\Debug\Project17.exe

# 특정 테스트만 실행
.\x64\Debug\Project17.exe --gtest_filter=ShellTest.*

# Shell(REPL) 대화형 실행
.\x64\Debug\Project17.exe --shell
```

#### Release 빌드 — Shell 자동 실행 (`main.cpp`)

```
msbuild Project17.vcxproj /p:Configuration=Release /p:Platform=x64

# 실행하면 곧바로 REPL 진입
.\x64\Release\Project17.exe
```

테스트 프레임워크: Google Test / GoogleMock 1.11.0 (NuGet)

---

## 디렉터리 구조

```
src/
├── common/       Token, Expr, Stmt 공유 타입
├── assembler/    Tokenizer, Parser
├── checker/      Checker
├── executor/     Executor, Environment
└── shell/        Shell (REPL)

test/
├── Tokenizer_test.cpp
├── Parser_test.cpp
├── Checker_test.cpp
├── Executor_test.cpp
├── Shell_test.cpp    — Shell 고유 동작 테스트 (실제 구현 사용)
└── Script_test.cpp   — 전체 파이프라인 End-to-End 통합 테스트
```

---

## 기능별 상세 분석

### 1. Tokenizer (어휘 분석)

**담당 파일**: `src/assembler/Tokenizer.h/.cpp`  
**인터페이스**: `tokenize(string) → vector<Token>`

소스 문자열을 한 번만 순회(`Scanner` 내부 클래스)하여 Token 목록을 생성한다.

#### 인식하는 토큰

| 분류 | 목록 |
|------|------|
| 키워드 | `var` `if` `else` `for` `true` `false` `and` `or` `print` |
| 단일 연산자 | `+` `-` `*` `/` `!` `=` `>` `<` `(` `)` `{` `}` `;` |
| 복합 연산자 | `==` `!=` `>=` `<=` |
| 리터럴 | NUMBER(double), STRING, IDENTIFIER |

#### 주요 처리 규칙

- **공백**: 스페이스·탭·`\r`은 무시, `\n`은 줄 번호(`line`) 증가
- **줄 주석**: `//` 이후부터 줄 끝(`\n`)까지 `skipLineComment()`로 건너뜀. `/` 단독은 나눗셈 연산자
- **문자열**: `"..."` — `lexeme`은 따옴표 포함, `literal`은 따옴표 제외
- **숫자**: 정수 및 소수점 형식 모두 `double`로 저장 (`"3.14"` → `3.14`)
- **식별자**: 영문자 또는 `_`로 시작, 이후 영숫자 또는 `_` 허용. 키워드 테이블에 일치하면 해당 키워드 토큰, 아니면 `IDENTIFIER`
- **키워드 접두사**: `variable`은 `IDENTIFIER`, `truex`는 `IDENTIFIER` (완전 일치만 키워드)

#### 오류 처리

| 상황 | 예외 메시지 |
|------|------------|
| 미지원 문자 | `"[line N] Unexpected character: X"` |
| 닫히지 않은 문자열 | `"[line N] Unterminated string"` |

---

### 2. Parser (구문 분석)

**담당 파일**: `src/assembler/Parser.h/.cpp`  
**인터페이스**: `parse(vector<Token>) → vector<StmtPtr>`

재귀 하향(Recursive Descent) 방식으로 Token 목록을 AST(`Stmt` 목록)로 변환한다.

#### 표현식 우선순위 (낮은 순 → 높은 순)

| 단계 | 메서드 | 연산자 |
|------|--------|--------|
| 1 | `assignment` | `=` (우결합) |
| 2 | `logicalOr` | `or` |
| 3 | `logicalAnd` | `and` |
| 4 | `equality` | `==` `!=` |
| 5 | `comparison` | `>` `>=` `<` `<=` |
| 6 | `term` | `+` `-` |
| 7 | `factor` | `*` `/` |
| 8 | `unary` | `!` `-` (우결합) |
| 9 | `primary` | 리터럴, 변수, `(expr)` |

#### 설계 포인트

- `parseBinaryLeft<NodeType>` 템플릿으로 좌결합 이진 표현식을 한 곳에서 처리
- `LogicalExpr`와 `BinaryExpr`를 별도 노드로 분리 — Executor에서 단락 평가(short-circuit) 구현을 위해
- 대입 좌변이 `VariableExpr`가 아니면 즉시 오류 (`"Invalid assignment target."`)
- `for` 문의 초기화·조건·증감식은 모두 선택적(nullable)

#### 오류 처리

모든 구문 오류는 `std::runtime_error("[line N] 메시지")` 형태로 던진다.

---

### 3. Checker (의미 분석)

**담당 파일**: `src/checker/Checker.h/.cpp`  
**인터페이스**: `check(vector<StmtPtr>) → void`

AST를 순회하여 실행 전에 검출 가능한 의미 오류를 `CheckError`(행 번호 포함)로 던진다.

#### 스코프 관리

스코프를 `vector<unordered_map<string, bool>>`로 관리한다. `bool` 값은 변수의 초기화 완료 여부(`false` = 선언만 됨, `true` = 초기화 완료).

```
declare(name)    → 현재 스코프에 false 등록 (중복 시 오류)
define(name)     → 현재 스코프의 값을 true로 변경
resolveVar(name) → 안쪽 스코프부터 탐색; false이면 자기 참조 오류
checkStmt(s)     → s.accept(*this) 래퍼
checkExpr(e)     → e.accept(*this) 래퍼
```

`ExprVisitor`·`StmtVisitor`를 `public` 상속하여 외부에서도 Visitor를 직접 활용할 수 있다.

#### 검출하는 오류

| 상황 | 오류 메시지 |
|------|------------|
| 같은 블록 내 변수 중복 선언 | `"변수 'X'이(가) 이미 이 블록에서 선언되었습니다."` |
| 초기화식에서 자기 자신 참조 (`var x = x;`) | `"자신의 초기화식에서 지역변수 'X'을(를) 읽을 수 없습니다."` |

#### 검출하지 않는 항목

- **미선언 변수 사용**: 글로벌 스코프 변수는 추적하지 않으므로 런타임에 Executor가 처리
- **대입 대상 변수 존재 여부**: `visitAssignExpr`에서 대입 대상(좌변)은 검사하지 않고 우변 값 표현식만 검사. 글로벌 변수 대입은 Executor가 처리
- **타입 오류**: 타입 검사는 Executor 담당
- **다른 블록 스코프에서의 동명 변수(shadowing)**: 정상 허용

---

### 4. Executor (실행)

**담당 파일**: `src/executor/Executor.h/.cpp`  
**인터페이스**: `execute(vector<StmtPtr>, ostream&) → void`

AST를 Visitor 패턴으로 순회하며 실행하고 결과를 `ostream`으로 출력한다.

#### 스코프 관리 (Environment)

```
Environment
├── values: map<string, LiteralValue>
└── enclosing: Environment*   ← 상위 스코프 포인터

define(name, val)   현재 스코프에 새 변수 정의 (shadowing 허용)
get(name)           현재 → 상위 재귀 탐색; 없으면 runtime_error
assign(name, val)   현재 → 상위 재귀 탐색 후 덮어쓰기; 없으면 runtime_error
```

블록·for 진입 시 새 `Environment`를 스택에 쌓고, `ScopeGuard` RAII로 블록 탈출 시 이전 환경으로 자동 복구한다.

#### 연산자 동작

| 연산자 | 동작 |
|--------|------|
| `+` | 두 피연산자가 모두 string이면 문자열 연결, 모두 number이면 덧셈, 혼합이면 오류 |
| `-` `*` `/` `>` `>=` `<` `<=` | number만 허용, bool·string이면 오류 |
| `==` `!=` | 타입 무관 동등 비교 (`std::variant` 비교) |
| `!` | truthy 판정 후 반전 |
| `-` (단항) | number만 허용 |
| `and` | 좌변이 falsy이면 좌변 반환, 아니면 우변 반환 (단락 평가) |
| `or` | 좌변이 truthy이면 좌변 반환, 아니면 우변 반환 (단락 평가) |

#### Truthy 규칙

`false`, `0.0`, `nil`(monostate)만 falsy. 문자열은 빈 문자열도 truthy.

#### 출력 형식 (`stringify`)

| LiteralValue | 출력 |
|--------------|------|
| `double` 정수 (`5.0`) | `"5"` |
| `double` 실수 (`3.14`) | `"3.14"` |
| `bool true` | `"true"` |
| `bool false` | `"false"` |
| `string` | 그대로 출력 |

#### 런타임 오류

| 상황 | 메시지 |
|------|--------|
| 미정의 변수 | `"Undefined variable 'NAME'."` |
| 0 나누기 | `"Division by zero."` |
| 단항 `-`에 비숫자 | `"Operand must be a number."` |
| `+` 타입 불일치 | `"Operands must be two numbers or two strings."` |
| 숫자 연산에 비숫자 | `"Operands must be numbers."` |

### 5. Shell (REPL)

**담당 파일**: `src/shell/Shell.h/.cpp`

4단계 파이프라인 전체를 묶어 한 줄씩 실행하는 REPL(Read-Eval-Print Loop)이다.

#### 공개 인터페이스

| 메서드 | 설명 |
|--------|------|
| `run(istream&, ostream&)` | `"> "` 프롬프트를 출력하며 입력 스트림에서 줄 단위로 읽어 실행 |
| `runLine(string) → string` | 한 줄을 실행하고 출력 결과를 문자열로 반환 (테스트 전용) |

#### 동작 방식

- 빈 줄은 파이프라인을 거치지 않고 즉시 반환
- 오류(`std::exception`) 발생 시 예외를 캡처하여 오류 메시지를 `ostream`에 출력 후 다음 줄 계속 처리 — Shell이 크래시되지 않고 다음 입력을 정상 처리함
- **실행 상태(변수 등)는 줄 간에 유지됨** — `Executor` 인스턴스가 Shell과 동일한 생명주기를 가지므로 `global_` 환경이 `runLine`/`run` 호출 사이에 유지됨

#### 테스트 구조

`Shell_test.cpp`는 Mock 없이 실제 구현체를 사용하여 Shell 고유 동작을 검증한다.  
전체 파이프라인 End-to-End 검증은 `Script_test.cpp`에서 수행한다.

---

