# TLショトカ移動2 AviUtl ExEdit2 プラグイン

タイムライン上のオブジェクト境界に移動する，いわゆる編集点移動のショートカットコマンドを追加するプラグインです．

他にも BPM グリッドへの移動や BPM グリッドの基準点を操作したり，スクロール操作などタイムラインをキーボード操作で見て回るのに便利なコマンドも追加しています．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl2_tl_walkaround2/releases) \[紹介動画準備中．\]

![タイムラインの現在フレームカーソルが編集点移動するデモ](https://github.com/user-attachments/assets/7192994f-8250-4ab0-ae71-ec1acd8c395a)

- AviUtl (無印) のプラグイン[「TLショトカ移動」](https://github.com/sigma-axis/aviutl_tl_walkaround)の AviUtl2 版として作成しました．一部機能は実装できていませんが，追加機能もあります．

##  動作要件

- AviUtl ExEdit2

  http://spring-fragrance.mints.ne.jp/aviutl

  - `beta25` で動作確認済み．

- Visual C++ 再頒布可能パッケージ (v14 の x64 版が必要)

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

## 導入方法

`tl_walkaround2.aux2` ファイルに対して，以下のいずれかの操作をしてください．

1.  AviUtl2 のプレビュー画面にドラッグ&ドロップ．
1.  プラグインフォルダにコピー．
    - AviUtl2 のメニューの「その他」 :arrow_right: 「アプリケーションデータ」 :arrow_right: 「プラグインフォルダ」で表示されます．

メインメニューの「ファイル」 :arrow_right: 「編集」 :arrow_right: 「TLショトカ移動2」にコマンドが追加されています．「ショートカットキーの設定」からこれらのコマンドにショートカットキーを割り当てて使用してください．

一部のコマンドは，レイヤー編集ウィンドウのオブジェクトを右クリックしたときの，コンテキストメニューからも実行できます．

メインメニューの「表示」にも「TLショトカ移動2」が追加されています．表示されるウィンドウで，一部のコマンドの挙動を調整できます．

##  各種コマンド

### 左/右の中間点(レイヤー)

現在選択フレーム移動のコマンドです．

現在選択中のオブジェクトのあるレイヤーから，現在選択フレームの左右にある最も近い中間点やオブジェクト境界を探して，そこに移動します．

- オブジェクトの終端では，そのオブジェクトの最終フレームの次のフレームに止まります．
- 現在選択中のオブジェクトがない場合，現在選択中のレイヤーから検索します．

### 左/右の境界(レイヤー)

現在選択フレーム移動のコマンドです．

[「左/右の中間点(レイヤー)」](#左右の中間点レイヤー)と同様ですが，オブジェクトの中間点は無視します．

### 左/右の中間点(シーン)

現在選択フレーム移動のコマンドです．

シーン全体から，現在選択フレームの左右にある最も近い中間点やオブジェクト境界を探して，そこに移動します．

- オブジェクトの終端では，そのオブジェクトの最終フレームの次のフレームに止まります．

### 左/右の境界(シーン)

現在選択フレーム移動のコマンドです．

[「左/右の中間点(シーン)」](#左右の中間点シーン)と同様ですが，オブジェクトの中間点は無視します．

### 左/右へ1ページ移動

現在選択フレーム移動のコマンドです．

現在選択フレームを，タイムラインの横幅で表示されている分だけ左右に移動します．

### 左/右へ一定量移動

現在選択フレーム移動のコマンドです．

現在選択フレームを，[「移動量(%)」](#移動量)で指定したページ割合の分だけ移動します．

- [「左/右へ1ページ移動」](#左右へ1ページ移動)の移動量を基準として，指定した割合だけ移動します．

### 選択オブジェクトへ移動

現在選択フレームと選択レイヤーの移動のコマンドです．

現在選択フレームを，現在選択中のオブジェクトが表示される最も近い位置に移動します．また現在選択レイヤーも同じオブジェクトのレイヤーに移動します．

### タイムラインの中央へ移動

現在選択フレーム移動のコマンドです．

現在選択フレームをタイムラインの中央に移動します．

### 左/右へ1ページスクロール

タイムラインの横方向のスクロールコマンドです．

タイムラインのスクロール位置を，タイムラインの横幅で表示されている分だけ左右に移動します．

### 左/右へ一定量スクロール

タイムラインの横方向のスクロールコマンドです．

タイムラインのスクロール位置を，[「移動量(%)」](#移動量)で指定したページ割合の分だけ移動します．

- [「左/右へ1ページスクロール」](#左右へ1ページスクロール)の移動量を基準として，指定した割合だけ移動します．

### 先頭/最後へスクロール

タイムラインの横方向のスクロールコマンドです．

タイムラインのスクロール位置を，シーンの冒頭または末尾に移動します．

### カーソル位置へスクロール

タイムラインの横方向のスクロールコマンドです．

現在選択フレームがタイムライン中央になるようにスクロールします．

### 選択オブジェクトへスクロール

タイムラインの縦と横のスクロールコマンドです．

現在選択中のオブジェクトが，タイムライン中央に表示されるようにスクロールします．

### 上/下へスクロール

タイムラインの縦方向のスクロールコマンドです．

タイムラインを上下に 1 レイヤー分スクロールします．

### 上/下へ1ページスクロール

タイムラインの縦方向のスクロールコマンドです．

タイムラインのスクロール位置を，タイムラインの縦幅で表示されている分だけ上下に移動します．

### 最上端/最下端へスクロール

タイムラインの縦方向のスクロールコマンドです．

タイムラインのスクロール位置を，1レイヤー目または最下段のレイヤーに移動します．

### 選択レイヤーへスクロール

タイムラインの縦方向のスクロールコマンドです．

タイムラインのスクロール位置を，現在選択レイヤーが中央に表示されるように移動します．

### 現在フレームのオブジェクトを選択(逆順)

選択オブジェクト変更のコマンドです．

現在フレームに表示されているオブジェクトを，下から上の方向に順繰りに選択します．

- AviUtl2 標準のコマンド「現在フレームのオブジェクトを選択」と，順序を除いては同じ機能です．

  AviUtl (無印) のこれに対応するコマンドは，Shift キーを押しているかどうかで順序が切り替わる仕組みでした．

### 左/右のオブジェクトを選択

選択オブジェクト変更のコマンドです．

現在選択オブジェクトの左/右にあるオブジェクトを選択します．

- 現在選択オブジェクトがない場合は，選択レイヤーの現在フレーム以降にある最初のオブジェクトを選択します．

- 標準の「前/次のオブジェクトを選択」とは異なり，現在フレームカーソルの移動は伴いません．

### 上/下のオブジェクトを選択

選択オブジェクト変更のコマンドです．

現在選択オブジェクトの上/下にあるオブジェクトを選択します．

- 選ばれるオブジェクトは「現在オブジェクトとタイミングが重なっているオブジェクトで，現在オブジェクトの中央に最も近いもの」です．

### 上/下のレイヤーを選択

現在選択レイヤーの移動のコマンドです．

現在選択レイヤーを上下に 1 レイヤー移動します．

### 左/右の小節線へ移動(BPM)

現在選択フレーム移動のコマンドです．

現在選択フレームの左右にある，最も近い BPM グリッドの小節線に移動します．

### 左/右の拍数線へ移動(BPM)

現在選択フレーム移動のコマンドです．

現在選択フレームの左右にある，最も近い BPM グリッドの拍数線に移動します．

### 左/右に1/N拍移動(BPM)

現在選択フレーム移動のコマンドです．

現在選択フレームの左右にある，最も近い BPM グリッドの $1/N$ 拍の境界線に移動します．この $N$ は[「BPM移動分母」](#bpm移動分母)で指定した値です．

- 例えば 3 連符の境界にオブジェクトを並べる際のガイド線として使えます．

### 左/右にグリッド基準線を移動(BPM)

BPM グリッドの変更コマンドです．

BPM グリッドの基準線を左右に 1 フレーム分移動します．

### グリッド基準線を現在フレームに(BPM)

BPM グリッドの変更コマンドです．

BPM グリッドの基準線を現在選択フレームに変更します．

### 最寄りの小節線を現在フレームに(BPM)

BPM グリッドの変更コマンドです．

BPM グリッドの基準線を移動して，現在選択フレームに最も近かった小節線が現在フレームと一致するようにします．

- [「グリッド基準線を現在フレームに(BPM)」](#グリッド基準線を現在フレームにbpm)と類似の挙動ですが，丸め誤差が違ってきます．

### 左/右に選択オブジェクトを詰める

選択オブジェクトを移動するコマンドです．

選択オブジェクト (複数選択オブジェクトか，現在オブジェクト設定に表示されているオブジェクト) を可能な限り，左または右に移動します．

- 標準の「左側に詰める」とは異なり，選択オブジェクトのみが移動します．
- レイヤー編集ウィンドウのオブジェクトを右クリックしたときの，コンテキストメニューからも実行できます．

### 上/下に選択オブジェクトを移動

選択オブジェクトを移動するコマンドです．

選択オブジェクト (複数選択オブジェクトか，現在オブジェクト設定に表示されているオブジェクト) を，上または下にレイヤー移動します．

### カーソル位置を元に戻す / カーソル位置をやり直す

現在選択フレーム移動のコマンドです．

現在フレームのカーソル位置の移動履歴から移動します．
1.  「カーソル位置を元に戻す」は，カーソル位置の履歴をさかのぼります．
1.  「カーソル位置をやり直す」は「カーソル位置を元に戻す」を取り消します．

カーソル位置の移動履歴は[「カーソル移動履歴取得頻度」](#カーソル移動履歴取得頻度)で指定された秒数ごとに観測・記録されていきます．履歴の最大個数は[「カーソル移動履歴記憶数」](#カーソル移動履歴記憶数)で指定します．

- 履歴の記録のタイミングは他にも，このプラグインのコマンドによる移動時にも起こります．

##  設定項目

メインメニューの「表示 :arrow_right: TLショトカ移動2」から表示されるウィンドウで，一部のコマンドの挙動を調整できます．

<img width="458" height="212" alt="設定 GUI のウィンドウ" src="https://github.com/user-attachments/assets/cc137ecd-d267-4bd4-af6d-db838ed1f46d" />

設定項目は `tl_walkaround2.ini` ファイルに記録されます．削除することで設定を初期化できます．
- プラグインフォルダにこのファイルが配置されます．
- 一部の設定はこのファイルを直接編集することでのみ変更できます．
  - 編集は AviUtl2 を終了した状態で行ってください．AviUtl2 の次回起動時に設定が反映されます．
- AviUtl2 終了時に生成されるので，このファイルがない場合は一度 AviUtl2 を起動 / 終了してください．

### 移動量(%)

[「左/右へ一定量移動」](#左右へ一定量移動)と[「左/右へ一定量スクロール」](#左右へ一定量スクロール)のコマンドでの移動量を，タイムライン横幅からの割合で % 単位で指定します．右にあるスライダーでも調整できます．

最小値は 0.1, 最大値は 100, 初期値は 25.

### BPM移動分母

[「左/右に1/N拍移動(BPM)」](#左右に1n拍移動bpm)のコマンドでの分母 $N$ を指定します．

最小値は 1, 最大値は 16, 初期値は 4.

### 範囲選択を抑制

このプラグインのコマンドを Shift キーを含むショートカットキーに割り当てて実行したとき，通常なら AviUtl2 の仕様上フレームの範囲選択が起こります．このチェックを ON にしていると，この範囲選択がこのプラグインのコマンドに限っては起こらなくなります．

初期値は OFF.

### 選択オブジェクトの追従

このプラグインのコマンドでフレーム移動したとき，移動先のフレームにあるオブジェクトを自動的に選択します．
- レイヤー編集ウィンドウのオプション「選択オブジェクトの追従」と同等の機能です．外部プラグイン経由でのフレーム移動にはこのオプションが影響しないため，プラグイン固有の設定項目を追加しています．

初期値は OFF.

### カーソル移動履歴取得頻度


[「カーソル位置を元に戻す」や「カーソル位置をやり直す」](#カーソル位置を元に戻す--カーソル位置をやり直す)のコマンドで利用するカーソルの移動履歴を，観測・記録する頻度を指定します．ここで指定した秒数ごとに履歴が記録されていきます．

指定は秒単位で 0.001 秒刻み，最小値は 0.5, 最大値は 60, 初期値は 2.

***この設定は `tl_walkaround2.ini` ファイルの以下の項目を直接編集することでのみ変更できます．***

```ini
[cursor_undo]
polling_period=2.000
```
### カーソル移動履歴記憶数

[「カーソル位置を元に戻す」や「カーソル位置をやり直す」](#カーソル位置を元に戻す--カーソル位置をやり直す)のコマンドで利用するカーソルの移動履歴の最大個数を指定します．

最小値は 16, 最大値は 16384, 初期値は 256.

***この設定は `tl_walkaround2.ini` ファイルの以下の項目を直接編集することでのみ変更できます．***

```ini
[cursor_undo]
queue_size=256
```

##  既知の問題

1.  スクロール系のコマンドを含め，ほとんどのコマンドはプレビュー再生中に実行するとプレビューが停止します (beta24a).

1.  [「カーソル位置を元に戻す」や「カーソル位置をやり直す」](#カーソル位置を元に戻す--カーソル位置をやり直す)は，他のシーンに切り替えたときは正常に動作しない可能性があります．

##  改版履歴

- **v1.20 (for beta25)** (2025-12-??)

  - コマンド「左/右のオブジェクトを選択」を追加．
    - 標準の「前/次のオブジェクトを選択」とは異なり，現在フレームカーソルの移動は伴いません．
  - コマンド「左/右に選択オブジェクトを詰める」を追加．
    - 標準の「左側に詰める」とは異なり，選択オブジェクトのみが移動します．
  - コマンド「上/下に選択オブジェクトを移動」を追加．

- **v1.10 (for beta25)** (2025-12-20)

  - コマンドの登録先を「インポート」メニューから「編集」 :arrow_right: 「TLショトカ移動2」に変更．
    - この変更で，このプラグインのショートカットキー設定が引き継がれなくなります．必要に応じてプラグイン更新前にショートカットキー設定をメモしておくなどして記録して，更新後に再設定してください．
  - 「選択オブジェクトの追従」の設定を追加．レイヤー編集ウィンドウの同名オプションと同じ機能ですが，独立な項目です．
  - `beta25` での動作確認．

- **v1.00 (for beta24a)** (2025-12-19)

  - 初版．

##  ライセンス

このプログラムの利用・改変・再頒布等に関しては MIT ライセンスに従うものとします．

---

The MIT License (MIT)

Copyright (C) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://mit-license.org/


# Credits

##  AviUtl ExEdit2 SDK

http://spring-fragrance.mints.ne.jp/aviutl

```
---------------------------------
AviUtl ExEdit2 Plugin SDK License
---------------------------------

The MIT License

Copyright (c) 2025 Kenkun

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

#  連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://x.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social
