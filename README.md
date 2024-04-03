# jihanki-vision
A set of programs for processing and identifying images of Japanese drink vending machines, using OpenCV. Developed for use on http://sagasoda.com
- fixperspective: fixes the perspective and alignment of photographs, with some optimizations/assumptions for Japanese vending machines.
- extract_drinks: extracts images of the individual drink slots in the display window, and identifies the configurations (rows x columns).
- identify_drinks: identifies images of drink containers based on a collection of previously identified images.

All programs are at a "functional proof of concept" level, and are not consistently accurate. Use at your own risk. 

飲料自動販売機の画像を識別するプログラムです。対象は日本で使われている自販機に特化しています。
- fixperspective: 画像の遠近を修正。日本の自販機に特化した最適化が幾つか含まれています。
- extract_drinks: 商品（缶・ボトル）の画像を抽出し、商品サンプルの構成（列、行）を認識する。
- identify_drinks:画像から商品を認識する。

プログラムはいずれも「最低限に機能する」レベルで、精度や性能は保証しません。あしからず。
