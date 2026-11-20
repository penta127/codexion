# Codexion：採点項目別の説明と想定質問集

この文書は、採点表の順に「何を見せるか」「どう説明するか」「追加で何を聞かれそうか」を整理したものです。
回答例を暗記するより、根拠の関数を開いて、自分の言葉で処理を追えるようにしてください。

実装全体の解説は [implementation_ja.md](implementation_ja.md)、実行方法は [README.md](../README.md) にあります。
実演コマンドはリポジトリのルートで実行します。

## 0. 評価前に共有しておく条件

採点用の通常動作テストでは、coderを200人以下、時間の引数を60ms以上にします。
1人でのburnoutなど、最初から失敗することを期待するケースと、全員の完了を期待するケースを分けます。

C89については、ユーザーの指定で対応範囲から外しています。現在は `<stdbool.h>` と `bool` を使っているため、厳密なC89適合を主張できません。採点表がC89を必須にするなら、この点だけは説明で解消できず、別途コードの変更が必要です。

また、「テストでエラーが出なかった」と「全入力で正しいことが証明された」は異なります。
特にEDFの「あらゆる実現可能な条件でburnoutしない」という強い保証を、有限個の動作テストだけで証明したとは回答しないでください。

## 1. Preliminaries / Basics：何を作ったプログラムか

### 口頭での回答例

> coderと同じ数のdongleを円形に配置し、各coderを1本のpthreadで動かすシミュレーションです。
> coderは左右2本を取得してcompileし、返却後にdebug、refactorを行います。
> dongleにはcooldownがあり、要求順はFIFOまたはEDFのheapで管理します。
> 専用monitorがburnoutと全員の完了を監視し、終了後に全threadをjoinして資源を解放します。

### 根拠

- [main.c](../main.c)：入力検査、configへの保存、simulation呼び出し。
- [simulation.c](../src/a2/simulation.c)：初期化、thread作成、開始、join、cleanup。
- [coder_routine.c](../src/runtime/coder_routine.c)：coderの行動ループ。
- [Makefile](../Makefile)：実行ファイル名とビルドフラグ。

### 実演

```sh
make
./codexion 4 1500 100 100 100 3 60 edf
make
```

最初のmakeで `codexion` が作られ、2回目は変更がなければ再リンクしないことを見せます。

### Q. 引数はいくつですか。なぜargcは9ですか。

A. 設定は8個です。`argv[0]` の実行ファイル名もargcに含まれるため9になります。

```text
人数 burnout compile debug refactor 必要回数 cooldown scheduler
```

### Q. 入力は何を検査していますか。

A. 個数、空文字、数字以外、int範囲超過、負数、人数0、不正schedulerです。
人数は1以上、ほかの数値は0以上を受け付けます。schedulerは小文字の `fifo` と `edf` だけです。

根拠は [error_check.c](../src/a1/error_check.c) の `error_check()`、`number_check()` と、[ft_atoi.c](../basic_code/ft_atoi.c) の桁追加前の範囲検査です。

```sh
./codexion 4 1500 100 100 100 3 60 invalid
./codexion 4 1500 -100 100 100 3 60 edf
```

### Q. なぜconfigとsharedを分けましたか。

A. configは起動引数の固定設定、sharedは実行中に変わる情報です。
設定値と、終了フラグ・時刻・要求キュー・mutexの役割を分け、誰が更新する情報かを追いやすくしています。

### Q. compileでは実際に何かをコンパイルしますか。

A. しません。dongleを使用中にして指定時間を待つことで、compileの所要時間と資源占有を再現します。

## 2. Basics：threadと共有データ

### 口頭での回答例

> 各coderに対してpthread_createを1回呼び、共通のcoder_routineを実行させます。
> 第4引数にそのcoderの構造体のアドレスを渡し、そこから共通sharedへアクセスします。
> monitorはcoderとは別に1本作成します。
> 各coderが最初の要求を登録し終わるまでmainが待ち、全員分を比較できる状態で共通の開始時刻を設定します。
> coderとmonitorは、その開始条件が整うまで待ちます。

### 根拠

[start_coders.c](../src/a3/start_coders.c) の `start_coders()`、`prepare_coder_start()`、`await_start()`、`finish_join_coders()`。
開始時刻の設定は [simulation.c](../src/a2/simulation.c) の `begin_simulation()` と `set_start_time()` です。

### Q. pthread_createの各引数は何ですか。

A. 第1引数はthread識別情報の保存先、第2引数は属性、第3引数は実行関数、第4引数は実行関数へ渡すデータです。
第2引数のNULLは通常の属性を使う指定です。

### Q. pthread_tとt_coderへのポインタは同じものですか。

A. 違います。`pthread_t` はjoinなどに使うthread識別情報で、`t_coder *` は自分のIDや状態を参照するためのデータです。

### Q. mainが順番にjoinすると、coderも順番に動きますか。

A. 動きません。joinは終了待ちです。作成済みのthreadは並行して実行され、mainだけがその終了を待ちます。

### Q. なぜ全員の初回要求を待つのですか。

A. threadを作成した順やCPUで実行された順だけで、最初の割当が決まることを避けるためです。
特にEDFの初回は全員のdeadlineが同じなので、全要求を登録してから比較し、高ID優先を実際の競合に反映します。
`prepared_count` が全員分になるまでmainが条件変数で待ち、その後にreadyを設定します。

### Q. 開始前に登録したEDF要求の時刻はどう扱いますか。

A. 登録時点ではcompile開始時刻が0なので、priorityはburnout_timeです。
共通開始時刻が決まると、全要求のpriorityへ同じstart_timeを加えます。
全要素に同じ値を足すのでheapの比較順は変わらず、正しい絶対deadlineになります。

### Q. 初回要求が二重にheapへ入ることはありませんか。

A. `queued` フラグで登録済みかを管理します。
初回にenqueueした要求はacquireで重ねて登録せず、割当時にqueuedをfalseへ戻します。

### Q. なぜ共有状態をローカル変数で持って大丈夫ですか。

A. `simulation()` 内のsharedは、全threadのjoin後まで存在します。
使っている途中にsimulationから戻らず、動的確保した配列もjoin後に解放するからです。

### Q. thread作成が途中で失敗したらどうしますか。

A. 終了状態を設定し、開始待ちのthreadを起こします。
`started_count` と `monitor_started` で作成済みのものだけをjoinし、その後に資源を解放します。

## 3. Global：変更可能なグローバル状態はあるか

### 口頭での回答例

> dongle、scheduler、ログを管理する変更可能なグローバル変数はありません。
> mainのconfigとsimulation内のsharedを出発点に、各threadへポインタで必要な状態を渡しています。

### 根拠と見せ方

[codexion.h](../codexion.h) は構造体と関数の宣言です。
実体はmain、simulation、各初期化関数で作っています。
`src/` と `basic_code/` を開き、ファイル直下に共有用の変数がないことを確認します。

### Q. static関数もグローバル変数ですか。

A. 違います。`static` を付けた関数は、そのファイルの中からだけ使える関数です。
一方、ファイルスコープのstatic変数やstaticローカル変数で共有状態を保持すれば、変更可能な共有状態になり得ます。この実装ではその方法を使っていません。

### Q. heapに置いた配列はグローバル変数ではありませんか。

A. mallocした領域であることと、グローバル変数であることは別です。
配列へのポインタはsharedが所有し、cleanupで解放します。

## 4. README.md：必要な説明はどこにあるか

### 口頭での回答例

> ルートのREADMEの先頭に、指定された作成者の文をイタリックで置いています。
> Description、Instructions、Resourcesに加えて、Blocking cases handledとThread synchronization mechanismsを用意しました。
> AIを使った箇所もResourcesに記載し、日本語の詳しいコード解説へリンクしています。

### 確認する場所

| 採点項目 | 見せる場所 |
|---|---|
| 指定された先頭行 | READMEの1行目 |
| 目的と概要 | Description |
| ビルド・実行 | Instructions |
| 参照資料とAI使用 | Resources |
| 並行処理上の問題と対策 | Blocking cases handled |
| mutex・条件変数・共有状態 | Thread synchronization mechanisms |

### Q. AIには何をさせましたか。

A. 初期化、失敗時cleanup、thread、heap、scheduler、monitor、Makefile、テストと解説の実装・レビューに利用しました。
生成したから正しいと考えず、コードの処理順とlockの範囲を確認し、動作テストやValgrind、Helgrindで検査しています。
まだ自分で説明できない部分は、理解済みと偽らず、その関数を開いて確認します。

## 5. Easy：burnoutせず必要回数を完了するか

### 口頭での回答例

> まず余裕のある時間条件で、全員が指定回数のcompileを完了して終了することを確認します。
> 回数はcompile開始時ではなく、終了してdongleを返す時点で加算します。
> 最後のcompileも所要時間が終わってから終了し、全threadをjoinします。

### 根拠

[coder_routine.c](../src/runtime/coder_routine.c) の `work_cycle()`、`finish_compile()`。
全員の達成判定は [monitor.c](../src/runtime/monitor.c) です。

### 実演

```sh
./codexion 2 2000 100 100 100 3 60 fifo
./codexion 5 2000 100 100 100 3 60 edf
./codexion 200 3000 60 60 60 3 60 edf
make test
```

### Q. なぜ人数と時間に制限を付けてテストしますか。

A. 採点表の条件に従い、200人以下・時間値60ms以上で通常動作を評価します。
小さすぎる時間値では、処理そのものよりOSの実行順や検査ツールの負荷の影響が大きくなります。

### Q. 必要回数に達したcoderは、その後どうなりますか。

A. 今回の実装では、そのcoderは退場し、以後はburnout監視から外れます。
全員が退場条件に達するまでは、ほかのcoderとmonitorが動き続けます。
このため、最後のcompileの後にdebugやrefactorログが出ないことがあります。

### Q. burnoutしたときは終了コード1ですか。

A. この実装では、burnoutはシミュレーションの終了結果なので0です。
入力不正、初期化失敗、thread作成など実行上のエラーでは1を返します。

## 6. Less easy：burnoutと時刻の境界

### 口頭での回答例

> burnoutの期限は、最後にcompileを開始した時刻にtime_to_burnoutを足した時刻です。
> まだ1回もcompileしていないcoderはsimulationの開始時刻を使います。
> monitorは共有状態をlockして検査し、burnoutを検出すると1回だけログを出して終了状態へ移します。
> 時刻はCLOCK_MONOTONICで測り、ログでは共通開始時刻との差を表示します。

### 根拠

- [monitor.c](../src/runtime/monitor.c)：期限判定と全体停止。
- [time.c](../src/runtime/time.c)：時刻取得と停止可能な待機。
- [log.c](../src/runtime/log.c)：停止状態と整合するログ。

### 実演

```sh
./codexion 1 120 60 60 60 2 60 fifo
./codexion 2 120 180 60 60 2 60 edf
```

前者はdongleが1本しかないのでcompileできません。
後者はcompileの180msがburnoutの120msを超えるので、compile中のburnoutを期待します。

### Q. なぜcompile完了時刻から測らないのですか。

A. 課題が「最後にcompileを開始してから」と定義しているからです。
完了時刻を使うと、compileにかかった時間だけ不当に猶予が延びます。

### Q. monitorより先に最後のcompileの完了処理が動くと、burnoutを見逃しませんか。

A. `finish_compile()` でも元のcompile開始時刻に対する期限を確認します。
期限へ達していた場合は完了回数を増やさず、そのcoderの処理を終えます。
その結果、monitorは未完了かつ期限切れとして検出できます。burnoutログを出すのはmonitorだけです。

### Q. 次のcompile開始時刻を更新して、すでに過ぎた期限を隠しませんか。

A. schedulerは、元のdeadlineより前であると確認した時刻を、そのまま割当とcompile開始時刻の記録に渡します。
確認後に別の時刻を取り直して期限判定と記録をずらさないようにしています。

### Q. compile中ならburnoutしないのですか。

A. compile中でも対象です。compile時間自体がburnout時間以上なら、新しいcompileの開始が間に合いません。

### Q. 0.5ms間隔で監視すれば、必ず0.5ms以内に検出しますか。

A. しません。0.5msは要求した待機時間で、実際にはOSの実行待ちやmutex取得待ちが加わります。
採点ではログから実際の遅延を計測し、要求された10msの許容と比較します。

### Q. 境界で1回だけ失敗したら、何を調べますか。

A. 最終compile開始時刻、burnout期限、実際のburnoutログ時刻を確認します。
同条件を複数回実行し、負荷や検査ツールの有無も比較します。
期限に余裕があるのに繰り返し失敗するなら、単に遅いPCだからとは説明せず実装を調べます。

### Q. 1人の場合、なぜ専用処理がありますか。

A. 左右が同じdongleになるためです。同じ通常mutexを自分で2回lockする処理には入れず、取得ログを1行出してmonitorによる停止を待ちます。

## 7. Less easy：資源の重複と状態遷移

### 口頭での回答例

> 2本の使用可否確認と割当を、共通状態mutexの下でまとめて行います。
> さらに各dongleの状態は専用mutexで保護し、使用中のものを別coderへ渡しません。
> compileでは2本を使用し、返却後はdebug、refactorへ進みます。

### Q. dongle mutexをcompile中ずっとlockしていますか。

A. いいえ。状態を確認・変更する短い区間だけlockします。
compile中の占有は `in_use = true` で表し、他の要求はその値を確認して待ちます。

### Q. 2本をまとめて渡す理由は何ですか。

A. 全員が1本ずつ持ったまま残り1本を待つ状態を避けるためです。
待機するcoderはdongleを持たず、使用中のcoderは時間が終われば2本とも返します。

### Q. mutexの取得順にも工夫がありますか。

A. 状態mutexの後に、小さいIDのdongle mutex、大きいIDのdongle mutexの順で取得します。
逆順で取得する経路を作らず、lock順序の循環を防ぎます。

### Q. Deadlockとstarvationは何が違いますか。

A. deadlockは互いの解放を待ち合って進めなくなることです。
starvationは全体が進んでいても特定のcoderだけが資源を得られないことです。
lock順序や2本一括割当で前者を防ぎ、要求の公平な順序付けで後者を抑えます。

## 8. Medium：cooldown

### 口頭での回答例

> dongleを返すときに、実際の返却時刻にcooldownを足した再使用可能時刻を保存します。
> 次に渡すときは、未使用であることと、現在時刻が再使用可能時刻以上であることの両方を確認します。
> 使用中フラグをfalseにするだけでは、すぐに渡しません。

### 根拠と実演

[dongles.c](../src/runtime/dongles.c) の `release_dongles()` と `dongles_available()`。

```sh
./codexion 2 2000 100 100 100 3 200 fifo
```

### Q. cooldownはcoderの休憩ですか。

A. 違います。各dongleの再使用制限です。
返したcoderがdebugしている間でも、別coderが使えるのはそのdongleのcooldown終了後です。

### Q. release_timeには返却時刻が入りますか。

A. 名前はrelease_timeですが、この実装で保存するのは「返却時刻＋cooldown」、つまり再使用可能時刻です。

### Q. ログだけでcooldownを完全に確認できますか。

A. 返却ログがないため、内部の実際の返却時刻は直接見えません。
compile開始＋compile時間＋cooldownから必要な下限を検査し、正確な代入・比較はコードでも確認します。

## 9. Medium：FIFOとEDFの違い

### 口頭での回答例

> FIFOは要求を受け付けた順、EDFはburnout期限が早い順です。
> 同じmin-heapを使い、要求に入れる比較キーを切り替えています。
> FIFOでは先行要求が待つdongleを後続へ渡さず、追い越しを防ぎます。
> EDFでは、2本とも使用可能な要求を期限順に割り当てます。別の使用中資源を待つ要求のために、空いている組を連鎖的に予約し続けることは避けます。

### 根拠

[request.c](../src/runtime/request.c) の `enqueue_request()`、[scheduler.c](../src/runtime/scheduler.c) のdispatch、[heap.c](../src/heap/heap.c) の `request_before()`。

### 実演

```sh
./codexion 5 2000 100 100 100 3 60 fifo
./codexion 5 2000 100 100 100 3 60 edf
make test
```

通常ログはOSの実行順にも左右されます。schedulerの差は、要求のdeadlineと到着順を固定した回帰テストも使って確認します。

### Q. FIFOはcoder番号順ですか。

A. 違います。共通mutexの下で要求を受け付けた順に `order` を割り当てます。
coder3が先に要求すれば、coder1より先に扱われます。

### Q. EDFのdeadlineは何ですか。

A. `s_compile_time + burnout_time` です。最後にcompileを開始した時刻が古いcoderほど、通常は早い期限になります。

### Q. EDFでは優先順位が高い要求は使用中のdongleも奪えますか。

A. 奪えません。compileは途中で取り上げない非プリエンプティブな処理です。
優先順位を使うのは、資源を新しく割り当てるときです。

### Q. EDFはheapの先頭が取れないと、全員が待ちますか。

A. 待ちません。先頭が必要とする2本のうち片方が使用中なら、その要求をpendingに置き、使用可能な組を持つ次の要求を調べます。
これは「現在割当可能な要求の中でdeadline優先」という扱いです。
FIFOの先行要求保護とは意図的に異なるため、両方の挙動を別のテストで確認します。

### Q. blockedとin_useは何が違いますか。

A. in_useは実際の使用中状態、blockedはFIFOの同一dispatch内で追い越しを防ぐ論理予約です。
blockedはmutexを保持することではなく、次のdispatchで作り直します。

### Q. heap配列は完全にソートされていますか。

A. いいえ。親が子より先に来る関係だけを保っています。
先頭が最優先で、pop後に末尾を先頭へ移し、子と比較してheapを修復します。

### Q. heapを使う利点と計算量は何ですか。

A. 要求の挿入と最優先の取り出しが通常O(log N)です。
この実装のdispatchは複数の要求をpopして戻すため、1回全体ではO(N log N)になります。

### Q. pendingはもう1つのheapですか。

A. 違います。今回渡せなかった要求の一時置き場です。
pop直後に同じheapへ戻すと、同じ先頭を繰り返し取り出してしまうため、1パス後にまとめて戻します。

### Q. EDFなら、実現可能な全入力で絶対にburnoutしませんか。

A. その無条件の保証を、テストだけで証明したとは言えません。
この実装は使用可能な組に対してdeadline順に割り当て、不要な予約による待ち時間を避けています。
ただし、資源の円形配置、非プリエンプティブなcompile、cooldown、OS実行順が絡むため、全実現可能入力への保証には別の証明が必要です。
課題のその要求に対し、検査した範囲の結果と未証明の範囲を区別して説明します。

## 10. Medium：refactorの時間とログの直列化

### 口頭での回答例

> refactorの開始ログを出してから、指定したrefactor時間が経過するまで待ちます。
> 終了状態は待機中も確認します。
> すべての状態ログはログmutexの下で出し、停止確認から出力まで状態mutexも保持します。
> そのため、行の混在だけでなく、burnout後の通常ログも防げます。

### 根拠

[coder_routine.c](../src/runtime/coder_routine.c)、[time.c](../src/runtime/time.c)、[log.c](../src/runtime/log.c)。

### Q. printfを1回で呼べばmutexはいりませんか。

A. この課題ではログ出力mutexを明示的に使います。
また、printf内部の直列化だけでは、停止確認とログの間にmonitorが停止させる競合を防げません。
状態確認とログを同じ状態lockの区間に入れる必要があります。

### Q. sleep中も状態mutexを保持していますか。

A. 保持しません。sleep前に解放し、起きてから再取得します。
保持したまま待つと、monitorもほかのcoderも共有状態を更新できなくなります。

### Q. データ競合にvolatileを使えば十分ですか。

A. 十分ではありません。volatileはthread間の相互排他や同期を保証しません。
共有変数の読み書きをmutexなどの同期機構で調整します。

### Q. なぜmutexが複数ありますか。

A. 状態の整合性、各dongleの状態保護、出力の保護という責任を分けています。
現在は状態mutexで広く直列化していますが、dongleとログの専用mutexも課題の指定に沿って用意し、取得順を統一しています。

## 11. EDF recode：同deadlineで高いcoder_idを優先する

### 口頭での回答例

> 現在のコードには、すでに高ID優先の変更を入れています。
> EDFの要求にtie_breakerとして-idを保存し、deadlineの次、arrival orderより前に比較します。
> 同deadlineでcoder1が先に要求しても、後から要求したcoder2が同じ資源を待っていれば、coder2が先になります。
> FIFOのtie_breakerは0のままなので、FIFOは到着順を維持します。

### 比較する順序

```text
FIFO：orderの小さい順
EDF ：deadlineの小さい順 → coder_idの大きい順 → orderの小さい順
```

### 変更を実演するとき

1. [request.c](../src/runtime/request.c) のEDF用tie_breaker代入を開く。
2. [heap.c](../src/heap/heap.c) で、tie_breakerがorderより先に比較されることを示す。
3. 低ID優先版を実演するなら、EDFのtie_breakerを `id` にする。
4. 高ID優先へ戻すときは `-id` にする。
5. 変更後にmakeして、意図する同点規則の競合テストを実行する。

`make test` の回帰テストは高ID優先を期待しています。低ID版のままでは失敗するのが正しい結果です。
実演前に作業を保存し、評価後にどちらの仕様へ戻すかを確認してください。

### Q. request_beforeの最後のid比較を逆にするだけではだめですか。

A. orderがidより先に比較され、各要求のorderが異なるからです。
そのままでは同deadlineでも到着順で決まり、最後のid比較に到達しません。
IDの比較キーをorderより前へ入れる必要があります。

### Q. 単に最初のログが高IDなら成功ですか。

A. それだけでは不十分です。
同じdeadline、共有資源を使う要求、逆の到着順を用意し、その条件で高IDが勝つことを確認します。
通常実行のthread開始順だけに依存した確認は避けます。

### Q. IDの符号を反転してオーバーフローしませんか。

A. 入力で人数は1以上かつintの正の範囲に制限しています。
IDも1〜INT_MAXなので、その負数はintに収まります。INT_MINを反転する処理ではありません。

## 12. メモリ・競合検査で聞かれたとき

### 口頭での回答例

> 通常終了とburnout終了を分けてMemcheckを実行し、確保した配列、heap本体と内部配列を解放することを確認します。
> Helgrindでは共有状態への競合とlock順序を確認します。
> 初期化途中とthread作成途中の失敗処理は、作成済み資源だけを片付ける構造にしています。

### 実演

```sh
valgrind --leak-check=full --error-exitcode=99 ./codexion 5 3000 60 60 60 2 60 edf
valgrind --leak-check=full --error-exitcode=99 ./codexion 2 120 180 60 60 2 60 fifo
valgrind --tool=helgrind --error-exitcode=99 ./codexion 5 3000 60 60 60 2 60 edf
```

### Q. freeとpthread_mutex_destroyは何が違いますか。

A. freeはmallocしたメモリの解放、destroyは初期化したmutexの使用終了です。
dongle配列をfreeする前に、その中の初期化済みmutexをdestroyします。

### Q. heapをfreeするのは1回ですか。

A. heap内部のdata配列とheap本体の2つがあります。`free_heap()` が両方を解放します。

### Q. 途中でmutex初期化が失敗したら、全mutexをdestroyしますか。

A. 成功したものだけです。未初期化のmutexや、初期化に失敗したmutexをdestroyしないようにします。

### Q. ValgrindとHelgrindが通れば、完全に正しいですか。

A. いいえ。実行した経路で検出されなかったという証拠です。
別のthread実行順でだけ発生する不具合もあるため、複数条件での実行とコード上の同期確認を組み合わせます。

## 13. Ratings：最後に何を回答・確認するか

### 口頭での回答例

> 通常動作、burnout、cooldown、scheduler、メモリと同期を、それぞれどのテストで確認したか説明できます。
> 今回の確認範囲と未証明の範囲を分けて伝え、recodeでは変更理由と結果を示します。

採点のflagは、説明のうまさではなく、実際のコードと実演結果に合わせて評価者と確認します。
コンパイル失敗やdata raceなどの停止条件を見つけた場合は、ほかが動いていても隠さずその項目を優先します。

最後に次の順番で、根拠を指しながら自分の言葉で話せるかを確認してください。

1. 実行コマンドからconfigに値が入るまで。
2. coder・dongle・heap・共通状態の初期化と関連づけ。
3. threadの作成、共通開始、最初の要求の扱い。
4. 1回のcompile取得からdebug、refactor、次の要求まで。
5. FIFOとEDFの比較キーと資源割当の違い。
6. burnoutを見つけたthreadから、全threadが終了するまで。
7. join後のmutex破棄とメモリ解放。
8. 同deadlineのrecodeを変える場所と、期待するテスト結果。

質問にすぐ答えられない場合は、推測で断言せず、該当関数を開いて「ここでこの値を更新しています」と確認して説明してください。
