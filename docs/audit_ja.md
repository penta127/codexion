# Codexion：要件・採点・解説の監査結果

確認日：2026年11月20日。対象は、この確認時点の作業ツリーです。
[課題文](subject_ja.md) と、チャットで提示された採点表を基準に確認しました。

## 1. 判定の範囲

具体的に再現した不具合を修正し、通常動作・期限境界・資源の後片付け・解説の整合を確認しました。
有限のテストを根拠に「あらゆる入力・OS負荷で満点」とは判定しません。

C89対応は依頼者の指定で対象外です。現在のboolは厳密なC89では使えません。
採点側がC89を必須にする場合は、その項目に限っては未適合です。

EDFは「2本とも取得できる要求の中で、最も早いdeadlineを選ぶ」と解釈しています。
FIFOは同じdongleを待つ先行要求を保護し、EDFは取得不能要求の予約で全体を止めない方式です。
この解釈を [回答集](defense_qa_ja.md) でも明示しています。

## 2. 採点項目との対応

| 項目 | この監査での結果・根拠 |
|---|---|
| C・指定警告フラグ | makeのビルド・再ビルドを確認。C89は対象外 |
| binary名codexion | MakefileのNAMEで指定 |
| 必須8引数・順序・fifo/edf限定 | error_check/inputと入力エラーテストで確認 |
| 1 coder＝1 thread | start_codersの作成ループ。追加でmonitorを1本作成 |
| 共有資源管理の変更可能globalなし | production sourceを確認。sharedはsimulationのローカル実体 |
| root READMEと指定セクション | 存在と内容、先頭のitalic文を確認 |
| Easy：200人以下で全員完了 | FIFO/EDF各2・5・50・200人、各行動60ms・cooldown60msで確認 |
| Less easy：burnout・10ms許容 | 1人、compileがburnoutより長いケースをログで確認 |
| dongle重複・状態遷移 | lock設計、単体テスト、ログからの占有時間下限検査を併用 |
| cooldown | 返却時刻＋cooldownで再使用を制限。取得間隔の下限を確認 |
| FIFO/EDFの差 | 本番enqueue_request経由の単体テストで、先行保護と取得可能要求優先を区別 |
| refactor時間・ログ直列化 | 状態遷移・時間・1行形式の検査とmutex設計を確認 |
| EDF同deadlineで高ID優先 | tie_breaker=-idをorderより前で比較。単体・初回割当で確認 |
| memory leak・data race・lock順 | Memcheck/Helgrindの検査ケースでエラー0 |
| 途中初期化・thread作成失敗 | 17箇所を意図的に失敗させ、異常終了・無出力・残存allocation0を確認 |
| 最後のcompileの完了待ち | 実行時間が最終開始時刻＋compile_time以上か検査 |

## 3. 見つけた問題と修正

### 3-1. EDFの予約連鎖で実現可能な入力でもburnoutした

旧コードでは、FIFOとEDFの両方で、渡せない要求のdongleをblockedにしていました。
さらに、全員の初回要求がそろう前に最初に起きたthreadから割り当て、並行数を減らしていました。

旧コードで再現した入力：

```sh
./codexion 8 300 60 60 60 3 60 edf
./codexion 10 800 200 200 200 3 60 edf
```

前者は偶数・奇数の2群を120ms間隔で交互に動かせば、各coderの開始間隔240msで実行できます。
後者も2群を260msずらして各群600ms周期で動かせるので、単なる不可能入力ではありません。

修正は次の4点です。

1. 各coderが初回要求を登録するまで、mainが開始時刻の設定を待つ。
2. 全初回deadlineを共通の開始時刻で補正する。
3. FIFOだけblockedで先行要求を保護する。
4. EDFは取得不能要求を保留し、取得可能な要求をdeadline順に割り当てる。

修正後は上記をmake testの回帰ケースに追加し、独立した繰り返し検査でも完了を確認しました。

### 3-2. deadline検査と開始時刻更新が異なる時刻を使っていた

旧コードでは、deadline100の直前99で検査した後、時刻を取り直して101を新しいcompile開始時刻にできました。
これでは期限切れが新しい時刻で隠れます。

現在は期限内と確認した同じ時刻をgrantとログへ渡します。
その時刻をシミュレーション上の割当確定・compile開始とし、別の現在時刻へ置き換えません。
固定時計の単体テストも追加しました。

### 3-3. monitorより先に期限切れcompileを完了扱いできた

旧finish_compileはstoppedだけを見て回数を増やしていました。
monitorがまだ動いていないと、deadline超過でも必要回数達成として監視対象から外れる可能性がありました。

現在は回数更新前に期限も検査します。
期限切れならdongleを返しますが回数は加算せず、monitorがburnoutを出せる状態を残します。
期限直前・ちょうど期限・期限直後を分けて検査します。

### 3-4. テストと解説の不足

- schedulerテストの独自の要求生成を、本番enqueue_requestへ変更。
- 開始回数に加え、最終compile前にプロセスが戻っていないかも検査。
- 採点用の既定テストを時間60ms以上へ統一。短時間・0境界は任意extendedへ分離。
- 初期化失敗検査をmake test-failuresで再現可能に変更。
- READMEと日本語解説を、初回バリア・FIFO/EDFの相違・時刻境界に整合。

## 4. 再確認コマンド

```sh
make
make test
make test-failures
norminette codexion.h main.c basic_code src tests
valgrind --leak-check=full --error-exitcode=99 ./codexion 5 3000 60 60 60 2 60 edf
valgrind --tool=helgrind --error-exitcode=99 ./codexion 4 3000 60 60 60 2 60 fifo
```

make testはscheduler単体、固定時計の期限境界、通常12ケース、tight EDF回帰4ケース、入力エラーを検査します。
make test-failuresはmalloc5箇所、mutex初期化6箇所、条件変数初期化1箇所、thread作成5箇所の計17ケースです。
Valgrindがなければ失敗終了だけを検査し、メモリ検査は明示的にSKIPします。

tests/fault_injection.cのstaticカウンタは、失敗させる呼び出し番号を数える**テスト専用バイナリの状態**です。
productionのcodexionにはリンクせず、共有資源・scheduler・ログを管理するglobalには使っていません。
test_deadlines.cの時計差し替えもテスト用リンクに限られます。

## 5. 解釈・保証を分けて説明する箇所

1. **全実現可能入力でのEDF成功**：以前の具体的な失敗例は修正済みですが、任意の入力・負荷に対する数学的証明はありません。課題の強い保証に対する未証明部分を、テスト通過で埋めたとは説明しません。
2. **取得不能要求の優先度**：EDFは取得可能な要求間で順序を守ります。取得不能な要求の空きdongleも常に予約せよ、という解釈とは一致しません。
3. **quota達成後**：自分の必要回数を完了したcoderは退場し、その後はburnout対象から外します。全員完了まで達成済みcoderも行動し続ける方式ではありません。READMEと解説に明示しています。
4. **10msの精度**：検査ケースでは許容内でした。OSが長時間threadを実行しない状況や、出力先の停止までは制御できません。
5. **検証の範囲**：Memcheck/Helgrindの成功は検査した実行の証拠であり、全入力の不具合不存在の証明ではありません。

## 6. 評価前の読む順番

1. [実装解説](implementation_ja.md)：要件からコードへの対応を理解する。
2. [採点項目別の回答集](defense_qa_ja.md)：根拠の関数を開きながら回答する。
3. この監査結果：確認済み・実装上の選択・未証明を区別する。

回答を暗唱するより、実装箇所を示しながら処理順とlockの役割を説明できることを目標にしてください。
