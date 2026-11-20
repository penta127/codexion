# Codexion：課題の要件とコードの読み方

この文書は「何を作る課題なのか」「今の実装は何をしているのか」を、コードを読みながら理解するための解説です。
課題の要求と、今回選んだ実装方法は分けて説明します。

最初から全関数を暗記する必要はありません。まず実行全体の流れを追い、その後に初期化、行動、scheduler、monitorを読む順番がおすすめです。

採点表に沿った口頭回答例と実演コマンドは [defense_qa_ja.md](defense_qa_ja.md) にあります。
C89対応はユーザー指定で今回の対象外です。現在はboolを使用しており、厳密なC89適合を主張する文書ではありません。

## 1. このプログラムで再現すること

複数のcoderが円形に座り、隣同士でdongleを共有します。
各coderは次の行動を繰り返します。

```text
左右のdongleを要求する
  ↓ 2本とも使えるようになった
compileする（2本を使う）
  ↓ compileが終わった
2本を返す（各dongleのcooldown開始）
  ↓
debugする（dongle不要）
  ↓
refactorする（dongle不要）
  ↓
次のcompileを要求する
```

compileは実際にCのコンパイラを実行する処理ではありません。
この課題では、dongleを使用中にして指定時間を待つことでcompileを表します。debugとrefactorも同様です。

前回compileを開始してから次のcompileを開始するまでに、`time_to_burnout` 以上経過するとburnoutです。
「compile完了からの経過時間」ではない点が重要です。compile中でも、その時間を超えればburnoutします。

## 2. 引数とconfigの対応

例として、次のコマンドを考えます。

```sh
./codexion 4 1000 100 100 100 3 60 edf
```

| 順番 | 課題の引数名 | configのメンバ | この例での意味 |
|---|---|---|---|
| 1 | number_of_coders | coders_number | coderとdongleが各4個 |
| 2 | time_to_burnout | burnout_time | compile開始の間隔は1000ms未満 |
| 3 | time_to_compile | compile_time | compileに100ms |
| 4 | time_to_debug | debug_time | debugに100ms |
| 5 | time_to_refactor | refactor_time | refactorに100ms |
| 6 | number_of_compiles_required | required_number | 全員が3回完了すれば終了 |
| 7 | dongle_cooldown | cooldown_dongle | 返却後60msは再使用不可 |
| 8 | scheduler | scheduler | EDFで要求の優先順位を決める |

`main.c` → `error_check()` → `input()` の順に処理します。

1. `error_check()` が `argc == 9` を確認します。実行ファイル名も含むので9です。
2. 数値7個が空でなく、数字だけで構成され、intの範囲内か確認します。
3. 人数は1以上、ほかの数値は0以上とします。
4. schedulerが `fifo` または `edf` か確認します。
5. `input()` が値を `t_config` に保存します。

数値変換は [ft_atoi.c](../basic_code/ft_atoi.c) にあります。
次の桁を加える前にintの範囲を超えないか判定し、超える場合は `-1` を返します。
入力検査は負数を受け付けないので、この `-1` をエラーとして扱えます。

**t_configは、ターミナルから受け取った設定だけを持ちます。**
終了フラグ、mutex、開始時刻などは入れていません。

## 3. 要件がどのファイルに対応するか

| 課題で必要なこと | 主なファイル・関数 |
|---|---|
| 引数の検査と保存（A.1） | `src/a1/`：`error_check()`, `input()` |
| coderの初期化（A.2） | `src/a2/coder.c`：`init_coder()` |
| dongleとそのmutexの初期化（A.2） | 同上：`init_dongle()` |
| ログmutex・終了状態の準備（A.2.8–A.2.9） | `src/a2/shared.c`：`init_shared()` |
| 時刻の準備（A.2.10） | `begin_simulation()`, `src/runtime/time.c` |
| 失敗時の後片付け（A.2.11） | `cleanup_shared()`, 各初期化関数 |
| coder threadの作成（A.3） | `src/a3/start_coders.c`：`start_coders()` |
| monitor threadとburnout監視（A.4） | `start_coders()`, `src/runtime/monitor.c` |
| coderの行動 | `src/runtime/coder_routine.c` |
| FIFO／EDF | `src/runtime/request.c`, `scheduler.c`, `src/heap/` |
| dongleの取得・返却・cooldown | `src/runtime/dongles.c` |
| ログの直列化 | `src/runtime/log.c` |
| threadの終了待ち | `finish_join_coders()` |
| ビルド | ルートの `Makefile` |

## 4. まず実行全体の順番を見る

中心になるのは [simulation.c](../src/a2/simulation.c) です。

```text
main
 ├─ error_check：入力を検査
 ├─ input：configに保存
 └─ simulation
     ├─ shared全体をゼロ初期化
     ├─ init_shared：共通mutex・条件変数を初期化
     ├─ prepare_data：配列・heapなどを確保し、関連づける
     ├─ start_coders：coderをN本、monitorを1本作成
     │    ├─ coderはprepare_coder_startで初回要求を登録して開始待ち
     │    └─ monitorはawait_startで開始待ち
     ├─ begin_simulation：全coderの初回登録を待つ
     │    └─ set_start_time：共通時刻を設定し、全threadを開始可能にする
     ├─ finish_join_coders：全threadの終了を待つ
     └─ cleanup_shared：資源を破棄
```

`pthread_create()` はthreadの作成、`pthread_join()` は作成済みthreadの終了待ちです。
joinを呼んだ順番でcoderが動くわけではありません。coderとmonitorは、join中もそれぞれ並行に動きます。

## 5. 構造体は何を分担しているか

定義はすべて [codexion.h](../codexion.h) にあります。

### 5-1. t_coder：1人分の情報

| メンバ | 役割 |
|---|---|
| id | ログに表示する1〜Nの番号 |
| thread | pthread_createが書き込むthreadの識別情報 |
| count_compile | 完了したcompileの回数 |
| s_compile_time | 最後にcompileを開始した時刻 |
| left_dongle_number / right_dongle_number | 使用するdongle配列の添字 |
| granted | schedulerから2本を割り当て済みか |
| queued | 自分の要求がすでにheapへ登録されているか |
| shared | 全員共通の実行状態へのポインタ |

`thread` と `shared` は別物です。`pthread_t` に共有データのポインタを入れることはできません。

### 5-2. t_dongle：1本分の情報

| メンバ | 役割 |
|---|---|
| id | 1〜Nの番号。mutexの取得順にも使う |
| mutex | 使用状態と再使用可能時刻を保護する |
| release_time | 名前に注意：実際には「次に使用可能になる時刻」 |
| in_use | 現在compileに使用中か |
| blocked | FIFOの今回の割当判定で、先行要求のために後続へ渡さない印 |

`in_use` と `blocked` は異なります。
前者は実際の使用中状態、後者はschedulerの順番を守るための一時的な予約です。
`blocked` は各dispatchの最初にリセットされ、`mutex_stop` の下で読み書きします。EDFではこの予約を設定しません。

### 5-3. t_requestとt_heap：要求と、その順番

`t_request` は「どのcoderが、どの優先順位で待っているか」を表します。

| メンバ | 役割 |
|---|---|
| id | 要求したcoderの番号 |
| priority | EDFのdeadline。FIFOでは全員0 |
| tie_breaker | EDFでは `-id`。FIFOでは0 |
| order | 要求を受け付けた通し番号 |

`t_heap` は要求を入れる配列 `data`、使用数 `size`、最大数 `capacity` を持ちます。
各coderは同時に1件だけ要求するので、capacityはcoderの人数分です。

### 5-4. t_shared：1回の実行全体で共有する情報

| 分類 | 主なメンバ |
|---|---|
| 設定への参照 | config |
| 実行資源 | coders, dongles, heap, pending |
| 終了・開始状態 | stopped, ready, start_time |
| 同期 | mutex_stop, mutex_log, changed |
| 要求順 | next_order |
| thread作成状況 | started_count, monitor, monitor_started |
| 開始準備の人数 | prepared_count |

`pending` は、dispatch中に「今は渡せなかった要求」を一時保存する配列です。
別のthreadや、もう1つの優先度キューではありません。

## 6. 実体とポインタの関係

`simulation()` は次の変数を1個作ります。

```c
t_shared shared;
```

これは実体です。mallocしていないので、`free(&shared)` はしません。
一方、その中のcoder配列などはmallocしているのでfreeが必要です。

```text
mainのconfig ← shared.config
                    ↑
              共通のshared
                ↑  ↑  ↑
             coder1 coder2 monitor
```

`prepare_data()` の `shared->coders[i].shared = shared;` が、この接続を作ります。
全coderが別々のsharedを持つのではなく、同じ実体を参照します。

`simulation()` は全threadのjoinが終わるまで戻りません。
そのため、threadが使っている途中でローカル変数のsharedが消えることを防げます。

## 7. A.2：初期化で何を保証するか

1. `simulation()` がsharedを `memset` でゼロにします。この実行環境では未作成の配列ポインタはNULLとなり、作成済みthread数や準備済み人数も0になります。
2. `init_shared()` がログmutex、状態mutex、条件変数を順に初期化します。mutexはゼロ埋めだけでは初期化できません。
3. `init_coder()` が配列を確保し、ID・回数・左右dongleの対応などを設定します。
4. `init_dongle()` が配列を確保し、各dongleのmutexを初期化します。
5. `init_heap()` とpendingの確保で要求管理の領域を用意します。
6. 各coderに共通sharedへのポインタを渡します。

4人の場合の対応は次のとおりです。

| coderのID | 左dongleの添字 | 右dongleの添字 |
|---|---|---|
| 1 | 0 | 1 |
| 2 | 1 | 2 |
| 3 | 2 | 3 |
| 4 | 3 | 0 |

IDは1始まり、配列の添字は0始まりです。
最後の右側を0へ戻すことで円形になります。

## 8. A.3：threadを作ってから共通時刻で開始する

[start_coders.c](../src/a3/start_coders.c) の作成処理は、各coderについて次を実行します。

```c
pthread_create(&shared->coders[i].thread, NULL,
    coder_routine, &shared->coders[i]);
```

1. 第1引数は、作成したthreadの情報を書き込む場所です。
2. 第2引数のNULLは、標準のthread属性を使う指定です。
3. 第3引数は、作成されたthreadが実行する関数です。
4. 第4引数は、その関数に渡すデータです。ここでは「自分自身のcoder」です。

`coder_routine(void *arg)` は `arg` を `t_coder *` として受け取り、`coder->shared` から共通情報にアクセスします。
monitorには、全coderを見る必要があるためsharedを直接渡します。

作成されたthreadは、mainが次のthreadを作る前に実行される可能性があります。
そのため、coderは `prepare_coder_start()`、monitorは `await_start()` で開始条件を待ちます。

coder側の準備は次の流れです。

1. `enqueue_request()` で最初の要求を登録し、queuedをtrueにする。
2. `prepared_count` を増やし、mainへ準備完了を通知する。
3. `await_start()` でreadyまたはstoppedを待つ。

全threadの作成後、main側の `begin_simulation()` は全coderの初回登録がそろうまで条件変数で待ちます。
そろったら `set_start_time()` で次を行います。

1. 現在時刻を `start_time` に保存する。
2. 全coderの `s_compile_time` に同じ時刻を設定する。
3. EDFの場合、すでに登録した全要求のpriorityへstart_timeを加える。
4. `ready = true` にし、呼び出し元がbroadcastで開始待ちのthreadを起こす。

初回要求の登録時はs_compile_timeが0なので、EDFのpriorityはburnout_timeです。
全priorityへ同じ開始時刻を加えることで、比較順を変えずに絶対deadlineへ直せます。
全要求がそろってから割り当てるので、初回の同deadlineで高IDを優先する規則も反映できます。

これにより、まだ1度もcompileしていないcoderも「simulation開始時刻」を基準に監視できます。
ただし、全threadが同じ瞬間にCPUで動き始める保証ではありません。

## 9. mutexと条件変数は何のためか

### 9-1. mutex_stopは、終了フラグ以外も守っている

名前は `mutex_stop` ですが、今のコードでは実行全体の状態を守るmutexです。
終了フラグ、開始状態、回数、compile開始時刻、要求キュー、割当判定を保護します。

たとえば、monitorが最終compile時刻を読んでいる途中でschedulerがその値を書き換えると、判定が競合します。
両方が同じmutexを使うことで、この同時アクセスを防ぎます。

### 9-2. lockする順序を統一する

```text
状態を扱う処理：mutex_stop
dongleを扱う処理：mutex_stop → 小さいIDのdongle mutex → 大きいID
ログを出す処理：mutex_stop → mutex_log
```

逆順で状態mutexを取りに行く処理は作っていません。
dongleのmutexは状態の確認・更新の間だけlockし、compile中ずっとlockし続ける方式ではありません。
compile中の使用権は `in_use = true` で保持します。

### 9-3. changedを使う待機と、実行中の待機は異なる

開始前の `await_start()` は条件変数 `changed` で待ちます。
`pthread_cond_wait()` は待機中に `mutex_stop` を解放し、起きたら再取得します。
起きても開始可能とは限らないため、条件をwhileで再確認します。

実行中の `wait_change()` は次の処理です。

```text
mutex_stopをunlock → usleep(500) → mutex_stopをlock
```

`usleep(500)` は500msではなく、500マイクロ秒、つまり0.5msです。
この間にほかのcoderやmonitorが状態を更新できます。
実行中の待機は条件変数で寝ているわけではないため、broadcastで即座に起きる方式ではありません。

## 10. FIFO／EDFは「誰から判定するか」を決める

[request.c](../src/runtime/request.c) の `enqueue_request()` が要求を作ってheapへ入れ、
[heap.c](../src/heap/heap.c) の `request_before()` が順序を比較します。

初回要求は開始前に登録済みです。`acquire_dongles()` はqueuedを確認し、同じ要求を二重登録しません。
割当時はqueuedをfalse、grantedをtrueにします。返却後の次の周では、新しい要求を登録できます。

### 10-1. FIFO

全要求のpriorityとtie_breakerを0にします。
そのため、比較は実質的に `order` の小さい順になります。

```text
coder3：order=0
coder1：order=1
coder4：order=2
→ 判定する順番は3 → 1 → 4
```

到着順は、状態mutexの下で要求を受け付けた順です。
thread作成順やcoder番号順ではありません。

### 10-2. EDF

```text
deadline = 最後のcompile開始時刻 + burnout_time
```

このdeadlineが小さい要求を優先します。
現在の実装では、同点なら高いcoder_id、その後にorderで比較します。
採点表の「高ID優先に変更するrecode」を、すでに反映した状態です。

| coder | deadline | tie_breaker=-id | 到着順 |
|---|---|---|---|
| 1 | 1000 | -1 | 0 |
| 4 | 1000 | -4 | 1 |
| 2 | 900 | -2 | 2 |

この場合は `2 → 4 → 1` です。deadlineが最優先なので、IDが高いだけで追い越すことはありません。
同じdeadlineでは `-4 < -1` を利用して、高IDを先にします。

### 10-3. heapは完全な整列済み配列ではない

min-heapでは、親の要求が子より先に来るという関係を保ちます。
配列の先頭から順に見ても、全要求が優先順位順に並んでいるわけではありません。

`heap_push()` は末尾へ入れ、親と比較しながら上へ移動します。
`heap_pop()` は先頭を取り出し、末尾を先頭へ移してから `sift_down()` で下へ調整します。
繰り返しpopすることで優先順位順に取り出せます。

### 10-4. dispatch_requestsの流れ

1. 全dongleの `blocked` をfalseにする。
2. heapから優先順位順に要求をpopする。
3. 期限内で、2本とも未使用・cooldown終了・blockedでなければ割り当てる。
4. 渡せなかった要求はpendingへ置く。FIFOの場合だけ、その要求の2本をblockedにする。
5. 全要求を判定したら、pendingに残ったものをheapへ戻す。

先頭をすぐheapへ戻すと、同じ要求を何度もpopして先へ進めなくなります。
pendingは、それを防いで「1回のdispatchで各要求を1回ずつ判定する」ためにあります。

### 10-5. FIFOでblockedを使い、EDFでは予約しない理由

coder1が先にdongle1・2を要求したものの、dongle1が使用中で取れないとします。
後からcoder2がdongle2・3を要求した場合、空いているdongle2を先に取らせると、共有dongleの順番を追い越します。

FIFOでは、coder1にまだ渡せなくてもdongle1・2をblockedにし、後続のcoder2も待たせます。
先行要求と関係のない組なら使えます。ただし、予約が連鎖して後続も待つ場合があります。

これはmutexを握り続ける予約ではありません。使用中のcoderは通常どおり返却できます。
毎回のdispatchで到着順から予約を作り直します。

このFIFO方式は追い越しを防ぐ一方、空いているdongleが一時的に使われない場合があります。
同じ予約をEDFにも使うと、予約が連鎖して、並行に実行できる組まで待たされることがあります。

そこでEDFは予約を作らず、deadline順に要求を見て、2本とも使用可能な要求へ割り当てます。
期限が早い要求の片方が使用中なら、その要求はpendingへ残し、使用可能なほかの組を調べます。
すでにcompile中のdongleを取り上げるわけではありません。

これは「現在割当可能な要求の中でdeadline優先」という非プリエンプティブな方式です。
不要な予約による待機を避ける改善ですが、あらゆる実現可能な入力に対する無burnoutを数学的に証明したものではありません。
課題の強い保証と、確認済みの有限個のテスト結果は区別して説明します。

## 11. dongle取得とcompile開始を追う

[dongles.c](../src/runtime/dongles.c) の関数は、呼び出し元が `mutex_stop` をlockしていることを前提にしています。

1. schedulerが現在時刻nowを取得し、旧deadlineより前か、2本を使えるか確認する。
2. `grant_dongles()` が2本を使用中にし、coderの `granted = true` にする。
3. `log_compile_locked()` が検査に使ったnowを開始時刻として保存し、取得ログ2行とcompileログを出す。
4. 要求元のcoder threadが `acquire_dongles()` から戻り、compile終了時刻まで待つ。

**割当判定やcompile開始ログは、要求元以外のcoder threadが実行する場合もあります。**
dispatchは共通の要求一覧を見るためです。ログのIDは、処理を実行したthreadではなく、割り当てられたcoderのIDです。

現在の設計は「schedulerが割当を確定した時点」をcompile開始として扱います。
要求元のthreadがCPUへ戻ってきた時点から、さらにcompile_timeを丸ごと待つ方式ではありません。
期限検査後にnowを取り直さず、検査した時刻を引数で渡します。検査と記録の時刻をずらして、旧期限の超過を隠すことを避けています。

2本の確認から使用中への更新までは共通状態mutexで直列化されるため、途中に別threadが割り込んで同じdongleを割り当てることを防いでいます。

## 12. 時刻・cooldown・行動の待機

[time.c](../src/runtime/time.c) の `now_ms()` は `CLOCK_MONOTONIC` の時刻をミリ秒へ変換します。
日時を表示する時計ではなく、経過時間を測るための時計です。

```text
ログ時刻 = 現在時刻 - shared.start_time
compile終了予定 = coder.s_compile_time + config.compile_time
burnout期限 = coder.s_compile_time + config.burnout_time
```

内部の `s_compile_time` と `release_time` は、start_timeを引く前の同じ時刻基準で保存します。
表示用の経過時間と混ぜて比較しないことが大切です。

`wait_until()` は、目標時刻に達するか、stoppedになるまで小刻みに待ちます。
長い時間を1回のusleepで待たないため、debugやrefactor中でも終了を確認できます。

`release_dongles()` は、次の値を両方のdongleへ保存します。

```text
release_time = 実際に返却する時刻 + cooldown_dongle
```

たとえば返却が200ms相当、cooldownが60msなら、260ms相当から使えます。
compile終了予定時刻ではなく、実際の返却時刻が基準です。

## 13. coderの1周を読む

[coder_routine.c](../src/runtime/coder_routine.c) の `work_cycle()` を追います。

1. `acquire_dongles()`：未登録なら要求を登録し、割り当てられるまで待つ。
2. `wait_until()`：compile開始時刻＋compile_timeまで待つ。
3. `finish_compile()`：旧deadlineを確認し、2本を返却する。停止済みでも期限切れでもなければ完了回数を増やす。
4. 自分が必要回数へ達した場合、期限切れの場合、または停止済みなら、そのcoderは終了する。
5. debugログを出し、debug_time待つ。
6. refactorログを出し、refactor_time待つ。
7. 次の周へ進む。

回数はcompile開始時ではなく、完了時に加算します。
burnoutでcompile中に停止した場合も、まず使用中のdongleを返しますが、その中断されたcompileは回数に加えません。
monitorがまだ検出していなくても、完了処理時に旧期限へ達していれば加算しません。
未達回数のままcoderを終了させることで、monitorが期限切れを検出してburnoutログを出します。
これにより、最後のcompileの期限切れが「回数達成済みなので監視対象外」として隠れることを防ぎます。

今回の実装では、自分の必要回数に達したcoderは退場します。
そのため、最後のcompileの後にdebug・refactorログがない場合があります。
これは現在の実装で選んだ扱いで、monitorも退場済みcoderをburnout監視から外しています。

## 14. A.4：monitorの仕事

[monitor.c](../src/runtime/monitor.c) では、専用threadが次を繰り返します。

1. mutex_stopの下で、全coderの状態を見る。
2. 必要回数に達しているcoderは、完了済みとして数える。
3. 未完了のcoderで `now - s_compile_time >= burnout_time` なら、burnoutログを出す。
4. burnoutを検出した場合、または全員完了の場合、stoppedをtrueにする。
5. まだ終わらなければ、一度状態mutexを解放して0.5ms待ち、再確認する。

ログ出力からstopped更新まで同じmutexを保持します。
別threadはその間に通常ログを差し込めないため、burnout後の追加ログを防げます。

0.5msの待機は10ms以内の検出を目指す設計ですが、OSの実行待ちや出力先の詰まりまで保証する値ではありません。
境界条件は実際のログと負荷を見て判断します。

## 15. 1人の場合と0の扱い

| 入力 | 今のコードの動作 |
|---|---|
| 人数0 | 入力エラー |
| 人数1・必要回数が正 | 取得ログを1行出し、compileできずmonitorがburnoutを検出 |
| 必要回数0 | 初期化・解放を行い、threadやログを作らず正常終了 |
| burnout_timeが0・必要回数が正 | compile割当をせず、monitorがburnoutを検出 |
| cooldownが0 | 返却後、すぐ再取得可能 |
| 不正scheduler・負数・非整数・int範囲外 | simulationを開始せずエラー終了 |

1人の場合は左右が同じdongleです。2回lockしてしまうことを避けるため、通常の取得ループへ入らず別処理にしています。

## 16. 失敗した場合、誰が何を片付けるか

| 失敗箇所 | 後片付け |
|---|---|
| ログmutex初期化 | 作成済みmutexなし。失敗を返す |
| 状態mutex初期化 | 成功済みのログmutexを破棄 |
| 条件変数初期化 | 状態mutex・ログmutexを破棄 |
| dongle mutexの途中 | それ以前に成功したdongle mutexだけ破棄し、配列をfree |
| heap内部配列の確保 | heap本体をfree |
| prepare_dataのいずれか | cleanup_sharedで確保できた資源を解放 |
| coder／monitorのthread作成 | stoppedにして開始待ちを解除し、作成済みthreadをjoinしてからcleanup |

`started_count` は成功したcoder thread数、`monitor_started` はmonitor作成の成否です。
失敗したthreadや未作成のthreadをjoinしないために使います。

`free()` と `pthread_mutex_destroy()` は役割が異なります。
dongle配列をfreeする前に、その配列内の初期化済みmutexをdestroyします。
heapは内部配列と本体の2つを確保しているため、`free_heap()` が両方を解放します。

正常終了でも、全threadのjoinが先、共有資源の破棄が後です。

## 17. ログが混ざらない理由

[log.c](../src/runtime/log.c) は `mutex_log` の下でprintfします。
通常ログの `log_action()` は、その前にmutex_stopをlockしてstoppedを確認します。

`log_locked()` と `log_compile_locked()` の名前のlockedは、**呼び出し元がすでに状態mutexを保持している**という前提を表しています。
関数の中ではログmutexを取得します。

2つのmutexは役割が異なります。

1. 状態mutex：停止確認から状態変更・ログまでの整合性を保つ。
2. ログmutex：出力同士が混ざらないようにする。

これらの前提を無視して、別の場所から直接printfすると、停止後ログや時刻順の乱れを作る可能性があります。

## 18. 検証の方法と、何を確認しているか

```sh
make
./codexion 4 1000 100 100 100 3 60 edf
make test
make test-failures
```

| 検証 | 見ていること |
|---|---|
| make | 警告をエラーにしたビルド、実行ファイル名 |
| tests/test_scheduler.c | FIFO追い越し防止、独立した組の割当、EDFの高ID優先、deadline最優先、使用可能ペアの割当 |
| tests/test_deadlines.c | now_msをテスト用wrapperで固定し、旧期限ちょうど・超過・直前の完了処理、検査時刻の継承を確認 |
| tests/test_codexion.py | ログ形式、遷移、回数、行動時間、取得間隔、cooldown、burnout、入力エラー、初回EDF高ID優先、最後のcompile終了まで待つこと |
| 同Python内のdefense_cases | 200人以下・時間値60ms以上の12ケースと、余裕の小さいEDF条件を反復する4ケースの計16ケース |
| make test-failures | malloc・mutex・条件変数・thread作成を計17か所で失敗させ、終了コードとValgrindのリーク・エラーを確認 |
| Norminette | ソースの書式・関数長など |
| Valgrind Memcheck | 検査ケースの不正メモリアクセス・リーク |
| Valgrind Helgrind | 検査ケースのthread間競合・lock順序 |

`make test` の通常の時刻付き動作テストは、人数200以下・時間値60ms以上です。
短時間や0の境界値は `python3 -B tests/test_codexion.py --extended` を明示した場合だけ追加で動きます。これらは採点条件とは分けた任意の検証です。

初期化失敗テストは [fault_injection.c](../tests/fault_injection.c) と [test_failures.py](../tests/test_failures.py) に保存してあり、`make test-failures` で再実行できます。
wrapperは検証用binaryだけにリンクされ、通常のcodexionには入りません。
Valgrindがない環境では終了処理の実行確認だけを行い、メモリ検査はSKIPと表示します。SKIPをリークなしの確認済みとは扱いません。
また、最終compile時間の検査は、実行全体の経過時間と最終compile開始ログを比較します。

テストが通ることと、あらゆる入力・負荷で正しいことは別です。
特にタイミングの検査はログを基にしており、内部資源の状態を直接すべて観測しているわけではありません。
ログ検査、コードのlock確認、Helgrindなどを組み合わせて確認します。

## 19. 理解を進めるおすすめ順

1. `main.c` と `src/a1/`：コマンドがどの値になるか説明する。
2. `codexion.h`：設定・1人分・1本分・共通状態を見分ける。
3. `simulation.c` と `shared.c`：作成順と破棄順を紙に書く。
4. `start_coders.c`：threadへ何を渡すか、なぜ開始待ちが必要か説明する。
5. `coder_routine.c`：通常の1周と、途中停止の1周を追う。
6. `request.c`、`scheduler.c` と `heap.c`：架空の要求を3件並べて比較する。
7. `dongles.c`：使用中と予約、返却時刻とcooldownを区別する。
8. `monitor.c` と `log.c`：burnout判定から最後のログまで追う。

各段階で「この関数は何を受け取り、何を変更し、何を返すか」「どのmutexを誰が持っているか」を説明できれば、次へ進めます。

## 20. 評価で説明できるようにしたいこと

1. configにmutexを入れず、sharedを別にした理由。
2. coderのIDと配列の添字が1ずれる理由。
3. compile開始時刻を、開始時に全員同じ値へ設定する理由。
4. pthread_createの第1引数と第4引数の違い。
5. pthread_join前に共有配列をfreeしてはいけない理由。
6. 2本の割当をまとめて判定する理由。
7. dongle mutexをID順に取得する理由。
8. FIFOとEDFでrequestのどの値が変わるか。
9. 同deadlineで高いIDを優先するために、負のIDを使う理由。
10. blockedとin_useの違い。
11. burnout判定が最後のcompile完了時刻を使わない理由。
12. 最後のcompile後にdebugログが出ない場合がある理由。
13. wait_changeで、sleep前に状態mutexを解放する理由。
14. ログmutexだけでは停止後のログを防げない理由。
15. 初回要求を全員分そろえる理由と、queuedが防ぐ二重登録。
16. FIFOだけが予約を作り、EDFは使用可能な組を調べる理由。
17. 最後のcompileの期限切れを、完了回数へ加えない理由。

回答例と追加質問は [defense_qa_ja.md](defense_qa_ja.md) にまとめています。
説明に詰まったところを、その都度コードと一緒に確認する使い方を想定しています。
