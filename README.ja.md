# PSP CarDash

[English](README.md) | [简体中文](README.zh-CN.md) | [日本語](README.ja.md)

<p align="center"><img src="assets/icon.jpg" width="180" alt="PSP CarDash アイコン"></p>

Sony PSP を ESP32 と組み合わせ、リアルタイム車載ダッシュボードとして再利用するオープンソースプロジェクトです。

> **安定版ベースライン：v0.3 ESP32 Link**
>
> リファレンス車両：**Mercedes-Benz W210 E430 / M113 V8 4.3L**
>
> 実車の OBD / K-Line 接続はまだ有効化していません。現在の安定版では ESP32 がシミュレーションしたテレメトリを Wi-Fi → UDP → PSP で送信します。

## このプロジェクトについて

古い PSP には今でも十分に見やすい画面、物理ボタン、そして基本的なネットワーク機能があります。PSP CarDash は PSP 側を軽量な表示端末として使い、ESP32-S3 を現代的なセンサーや車両インターフェースとのブリッジとして利用します。

~~~text
車両 / OBD（将来）
        │
     ESP32-S3
        │
   2.4 GHz Wi-Fi
        │
     UDP 4210
        │
       PSP
~~~

## 現在の安定機能

v0.3 は実機 PSP でテスト済みです。

- Drive / Engine / Trip / Diagnostics の4画面
- ESP32-S3 SoftAP 接続
- 10 Hz UDP テレメトリ
- ESP32 がない場合のローカル Demo モード
- Demo / ESP32 データソースの実行時切替
- PSP Wi-Fi Profile 選択
- ESP32 リンク切断時の安全なフォールバック
- W210 E430 / M113 リファレンスプロファイル

## 必要なハードウェア

### 必須

- Homebrew が動作する Sony PSP
- ESP32-S3 開発ボード
- ESP32 書き込み用 USB ケーブル

### リファレンス構成

- PSP-3000 シリーズ
- ESP32-S3 DevKitC-1
- PSP の古い Wi-Fi 仕様との互換性を優先した 2.4 GHz オープンアクセスポイント

PSP の Wi-Fi スタックは古いため、互換性を優先してリファレンス ESP32 ファームウェアは**暗号化なしの 2.4 GHz AP**を作成します。

~~~text
SSID: PSP-CAR-DASH
AP IP: 192.168.4.1
UDP: 4210
~~~

## リポジトリ構成

~~~text
PSP-CarDash/
├── psp/            PSP Homebrew ソースと Makefile
├── esp32/          ESP32-S3 テレメトリブリッジ
├── tools/          PC 用 UDP シミュレータ
├── docs/           プロトコル、ハードウェア、ロードマップ
├── assets/         プロジェクト画像
└── experimental/   開発中の実験機能
~~~

## PSP 操作

| ボタン | 操作 |
|---|---|
| 左 / 右 | ダッシュボード画面切替 |
| START | ローカル Demo アニメーション ON/OFF |
| SELECT | 値をフリーズ / 再開 |
| L / R トリガー | 未接続時に PSP Wi-Fi Profile を選択 |
| △ | ESP32 Link 接続 / 切断 |
| ○ | 終了 |
| HOME | PSP システムから終了 |

## クイックスタート

### 1. PSP アプリをビルド

[PSPDEV](https://pspdev.github.io/) をインストールし、`psp-gcc` が利用できる状態にします。

~~~bash
export PSPDEV=/opt/pspdev
export PATH="$PSPDEV/bin:$PATH"

cd psp
make clean
make
~~~

`psp/EBOOT.PBP` を以下へコピーします。

~~~text
/PSP/GAME/PSPCARDASH/EBOOT.PBP
~~~

### 2. ESP32 を書き込む

Arduino IDE または Arduino CLI で以下を開いて ESP32-S3 に書き込みます。

~~~text
esp32/PSP_CAR_DASH_ESP32_LINK.ino
~~~

ESP32 は次の AP を作成します。

~~~text
SSID: PSP-CAR-DASH
Security: OPEN
UDP: 4210
Rate: 10 Hz
~~~

### 3. PSP に Wi-Fi Profile を作成

PSP XMB で以下を設定します。

~~~text
Settings
 -> Network Settings
 -> Infrastructure Mode
 -> New Connection
 -> Scan
 -> PSP-CAR-DASH
 -> Security: None
 -> Save
~~~

PSP CarDash を起動し、L/R トリガーで保存した Wi-Fi Profile を選択して **△** を押します。

正常に接続できると Diagnostics 画面に次のように表示されます。

~~~text
AP STATE   4 / 4
PSP IP     192.168.4.x
UDP PORT   4210
PACKETS    増加し続ける
~~~

## テレメトリプロトコル

v0.3 はシンプルな ASCII パケットを使用します。

~~~text
PCD1,seq,speed,rpm,coolant,throttle,voltage10,intake,load,fuel10,oiltemp,oilpress10
~~~

例：

~~~text
PCD1,145,86,3250,92,27,142,31,38,68,96,38
~~~

詳細は [docs/PROTOCOL.md](docs/PROTOCOL.md) を参照してください。

## PC からのテスト

ESP32 がなくても PC から UDP データを送信できます。

~~~bash
python3 tools/pcd_sender.py
~~~

## プロジェクト状態

### 安定版 — v0.3 ESP32 Link

- PSP ランタイム：テスト済み
- ページ切替：テスト済み
- ESP32 SoftAP：テスト済み
- UDP テレメトリ：テスト済み
- Demo フォールバック：テスト済み

### 実験版 — 高精細 Render UI

滑らかな 480×272 の高精細ダッシュボード UI を開発中です。実機で同等の検証が完了するまでは安定版ソースを置き換えません。

[experimental/render-ui/README.md](experimental/render-ui/README.md) を参照してください。

## ロードマップ

- [x] PSP 安定ランタイム
- [x] ESP32-S3 Wi-Fi ブリッジ
- [x] UDP テレメトリプロトコル
- [x] ローカル Demo / フォールバック
- [ ] 高精細レンダリング UI
- [ ] W210 診断コネクタと物理層の確認
- [ ] W210 標準 OBD-II / K-Line テレメトリ
- [ ] 実 RPM / 車速 / 冷却水温 / スロットル / 電圧
- [ ] DTC 読み取り
- [ ] トリップ記録
- [ ] 複数車種プロファイル

## 車両接続の安全性

現在の安定版コードは**車両制御コマンドを送信しません**。

将来の車両対応は、まず読み取り専用テレメトリから開始します。実験機器を車両の診断端子や通信バスへ接続する前に、ピン配置、プロトコル、電圧レベル、保護・絶縁回路を必ず確認してください。

## コントリビューション

Issue、実機テスト結果、車種別プロトコルアダプタ、UI 改善を歓迎します。

[CONTRIBUTING.md](CONTRIBUTING.md) を参照してください。

## ライセンス

コードは MIT License で公開しています。[LICENSE](LICENSE) を参照してください。

## 商標について

本プロジェクトは独立した非公式のオープンソース／ホビープロジェクトです。Sony、PlayStation Portable / PSP、Mercedes-Benz、W210 および関連する名称・ロゴは各権利者の商標です。本プロジェクトはいかなるメーカーとも提携・後援・公式承認関係にありません。
