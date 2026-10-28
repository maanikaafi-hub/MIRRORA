# MIRRORA — سورس اولیه به همراه GUI

این ZIP شامل سورس C++، رابط گرافیکی resizable، فایل CMake و اسکریپت‌های Build است.

## خروجی‌های هدف
- Windows x64: VST3
- macOS: VST3 و AU (Audio Unit)

AU روی ویندوز ساخته نمی‌شود؛ برای AU باید پروژه را روی macOS با Xcode کامپایل کنید. این فایل ZIP شامل سورس است، نه باینری آماده نصب.

## نیازمندی‌ها
- JUCE 7.0.12 یا نسخه سازگار جدیدتر
- CMake 3.22+
- Windows: Visual Studio 2022 با Desktop development with C++
- macOS: Xcode و Command Line Tools

JUCE را از منبع رسمی دریافت کنید و متغیر محیطی `JUCE_DIR` را به مسیر سورس آن تنظیم کنید.

## Windows x64 / VST3
در PowerShell، داخل پوشه پروژه:
```powershell
$env:JUCE_DIR = "C:\Libraries\JUCE"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target MIRRORA_VST3
```
خروجی معمولاً در `build/MIRRORA_artefacts/Release/VST3/MIRRORA.vst3` است.

## macOS / AU + VST3
در Terminal:
```bash
export JUCE_DIR="$HOME/Developer/JUCE"
cmake -S . -B build -G Xcode
cmake --build build --config Release --target MIRRORA_VST3 MIRRORA_AU
```

## امکانات موجود در prototype
- حالت‌های Simple Reverse و Rhythmic Reverse
- کنترل Mix، Length، BPM Sync و Output
- تلاش برای خواندن BPM میزبان
- GUI قابل تغییر اندازه
- `MaaNi KaaFi` در بخش About و `Plugin Store` پایین سمت راست

## محدودیت‌ها
این prototype اولیه و نیازمند تست/اصلاح است، نه نسخه آماده انتشار. الگوریتم reverse پنجره‌ای است و تأخیر برابر طول پنجره و آرتیفکت‌های احتمالی دارد. حالت Simple فعلاً پنجره ثابت نیم‌ثانیه‌ای دارد. Build روی این محیط اجرا نشده و سازگاری با نسخه/کامپایلر خاص تضمین نشده است.
