# ساخت خودکار MIRRORA با GitHub Actions

این پروژه شامل سورس اولیهٔ MIRRORA و workflow ساخت خودکار است. با اجرای GitHub Actions می‌توان خروجی‌های زیر را دریافت کرد:

- Windows x64: VST3
- macOS: VST3
- macOS: Audio Unit (AU)

## روش استفاده

1. فایل ZIP را استخراج کن.
2. محتوای پوشه را در یک repository جدید GitHub آپلود کن؛ فایل و پوشهٔ مخفی `.github` هم باید آپلود شود.
3. در GitHub وارد تب **Actions** شو و workflow با نام **Build MIRRORA Plugins** را باز کن.
4. روی **Run workflow** بزن؛ یا با push کردن کد به شاخهٔ `main` یا `master` ساخت به‌صورت خودکار اجرا می‌شود.
5. پس از اتمام موفق workflow، در پایین صفحهٔ اجرای workflow بخش **Artifacts** را باز کن و خروجی موردنظر را دانلود کن.

## نکات مهم

- GitHub Actions سورس JUCE را هنگام build دریافت می‌کند؛ لازم نیست JUCE را جداگانه داخل ZIP قرار بدهی.
- این بسته سورس و تنظیمات ساخت است، نه فایل پلاگین کامپایل‌شده.
- ساخت هنوز در این محیط اجرا و تأیید نشده است؛ ممکن است بسته به نسخهٔ JUCE یا خطاهای کامپایل، نیاز به اصلاح داشته باشد.
- خروجی Windows برای x64 است. خروجی macOS روی runner اپل ساخته می‌شود؛ این workflow فعلاً build جداگانهٔ Universal Binary را تضمین نمی‌کند.
