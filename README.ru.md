# Unlock UWB (Zygisk + LSPlant)

[English](README.md) | [Русский](README.ru.md)

Модуль Magisk / KernelSU / APatch для разблокировки функционала **Ultra-Wideband (UWB)** и инженерного меню **UWB Labs** на смартфонах Samsung Galaxy (One UI) и Android-устройствах.

---

## 📌 Что делает этот модуль

1. **Разблокировка меню и тумблера UWB в Настройках One UI**:
   - Обходит ограничения `isMenuUnavailable()`, `isRestrictionMode()` и `isRegulationMode`.
   - Принудительно включает поддержку в `com.samsung.android.settings.uwb.UwbPreferenceController` и AOSP-контроллере.
2. **Активация скрытого инженерного меню UWB Labs**:
   - Автоматически включает `mLabsEnabled` и системное свойство `uwb.labs.enable = true`.
   - Открывает доступ к встроенным тестам:
     - **FiRa One-to-One Ranging Test** (`UwbFiraTestFragment`);
     - **Simple Ranging Test** (`UwbSimpleTestFragment`);
     - **DL-TDoA Test** (`UwbDltdoaTestFragment`);
     - Расширенная статистика и история сессий.
3. **Обход регуляторных ограничений региона (каналы 5 и 9)**:
   - В системном сервисе (`system_server`) подменяет код страны на разрешённый (`US`) в `CountryDetectorService` и `UwbCountryCode`.
   - Внедряет флаг Samsung HAL: `uwb.regulation.skip = true` (пропуск проверки регуляторного домена на уровне UCI-прошивки чипа).
4. **Аппаратная фича**:
   - Включает системный дескриптор `android.hardware.uwb` через systemless overlay `/vendor/etc/permissions/android.hardware.uwb.xml`.

---

## 🛠 Архитектура

Модуль работает как **Zygisk-модуль**, использующий библиотеку **LSPlant** (тот же движок перехвата ART, что и в LSPosed), внедряясь напрямую при старте процессов:

* **`com.android.settings`**: Перехват и нейтрализация проверок доступности UWB.
* **`system_server`**: Перехват методов детекции страны.
* **Native / System Properties**: Установка проперти `uwb.regulation.skip` и `uwb.labs.enable` до инициализации служб.

---

## 🚀 Установка

1. Скачайте последний архив `unlock-uwb-v*.zip` из раздела **Releases** или артефактов **Actions**.
2. Установите архив в **Magisk**, **KernelSU** или **APatch**.
3. Убедитесь, что **Zygisk** включен в настройках вашего root-менеджера.
4. Перезагрузите устройство.
5. Откройте **Настройки** -> **Подключения** -> **Ultra-Wideband (UWB)**. Пункт станет активным и откроет экран **UWB Labs**.

> **Примечание:** Для физической работы UWB устройство должно иметь аппаратный чип UWB (на устройствах Samsung это линейки Plus / Ultra / Fold начиная с Galaxy S21+/S21 Ultra).

---

## 🏗 Сборка

### Автоматическая сборка (GitHub Actions)
В репозитории настроен GitHub Actions: каждый push в `main` собирает артефакт, а push тега версии (например, `v1.0.0`) собирает ZIP-архив с библиотеками для `arm64-v8a` и `armeabi-v7a` и публикует GitHub Release.

### Локальная сборка
Требуется:
- Android NDK (r26c или новее);
- Python 3;
- CMake 3.22+;
- Java JDK 17+.

```bash
# 1. Клонирование с рекурсивными субмодулями
git clone --recursive https://github.com/skb8/unlock-uwb.git
cd unlock-uwb

# 2. Компиляция и упаковка
export ANDROID_NDK_HOME=/path/to/android-ndk
python3 build.py
```
Готовый архив появится в каталоге `release/`.
