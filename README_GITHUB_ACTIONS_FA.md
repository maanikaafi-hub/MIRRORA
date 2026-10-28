# ساخت خودکار با GitHub Actions

فایل `.github/workflows/build.yml` روی هر push و هر Pull Request اجرا می‌شود و
سه کار (job) دارد:

| Job | سیستم | خروجی |
|---|---|---|
| `Windows x64 • VST3` | `windows-2022` | `MIRRORA-Windows-x64-VST3.zip` (شامل `MIRRORA.vst3`) |
| `macOS • VST3 + AU (universal)` | `macos-14` | `MIRRORA-macOS-VST3.zip` و `MIRRORA-macOS-AU.zip` (universal: arm64 + x86_64) |
| `Linux x64 • compile check` | `ubuntu-22.04` | فقط تست کامپایل VST3 (بدون انتشار فایل) |

هر سه job از JUCE نسخه‌ی `8.0.8` استفاده می‌کنند (متغیر `JUCE_TAG` در بالای فایل).

## دریافت فایل‌های ساخته‌شده
1. به تب **Actions** بروید و آخرین اجرای موفق را باز کنید.
2. پایین صفحه، بخش **Artifacts**:
   * `MIRRORA-Windows-x64-VST3`
   * `MIRRORA-macOS`
3. فایل ZIP را دانلود کنید.

## ساخت Release (لینک دانلود دائمی)
اگر یک تگ با الگوی `v*` بزنید، job چهارم (`release`) اجرا می‌شود و فایل‌های
ویندوز و مک را به‌صورت دارایی (asset) به همان Release متصل می‌کند:

```bash
git tag v1.0.0
git push origin v1.0.0
```

سپس در صفحه‌ی **Releases** مخزن، لینک‌های دانلود آماده‌اند.

## اجرای دستی
در تب Actions، ورک‌فلو **Build MIRRORA** را انتخاب و **Run workflow** را بزنید.

## نکته‌ها
* بیلد مک بدون امضا (unsigned) است؛ بعد از نصب یک بار
  `sudo xattr -cr <bundle>` را اجرا کنید.
* برای AU در Logic/GarageBand در صورت نیاز `auval -a` را اجرا کنید.
* AU فقط روی مک ساخته می‌شود؛ روی ویندوز امکان‌پذیر نیست.
