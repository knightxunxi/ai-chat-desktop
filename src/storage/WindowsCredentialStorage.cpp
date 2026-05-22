#include "storage/WindowsCredentialStorage.h"

#include <QByteArray>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincred.h>
#endif

namespace {

#ifdef Q_OS_WIN
constexpr wchar_t CredentialTargetName[] = L"AIChatDesktop/OpenAICompatibleApiKey";
constexpr wchar_t CredentialUserName[] = L"AIChatDesktop";

void setWindowsError(QString *error, const QString &operation, DWORD errorCode)
{
    if (error == nullptr) {
        return;
    }

    *error = QStringLiteral("%1 failed with Windows error code %2.")
                 .arg(operation)
                 .arg(static_cast<qulonglong>(errorCode));
}

QString apiKeyFromCredential(const CREDENTIALW *credential)
{
    if (credential == nullptr || credential->CredentialBlob == nullptr || credential->CredentialBlobSize == 0) {
        return {};
    }

    const QByteArray bytes(reinterpret_cast<const char *>(credential->CredentialBlob),
                           static_cast<qsizetype>(credential->CredentialBlobSize));
    return QString::fromUtf8(bytes);
}
#else
void setUnsupportedError(QString *error)
{
    if (error != nullptr) {
        *error = QStringLiteral("Windows Credential Manager is not available on this platform.");
    }
}
#endif

} // namespace

QString WindowsCredentialStorage::readApiKey(QString *error) const
{
#ifdef Q_OS_WIN
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(CredentialTargetName, CRED_TYPE_GENERIC, 0, &credential)) {
        const DWORD errorCode = GetLastError();
        if (errorCode == ERROR_NOT_FOUND) {
            return {};
        }

        setWindowsError(error, QStringLiteral("CredReadW"), errorCode);
        return {};
    }

    const QString apiKey = apiKeyFromCredential(credential);
    CredFree(credential);
    return apiKey;
#else
    setUnsupportedError(error);
    return {};
#endif
}

bool WindowsCredentialStorage::writeApiKey(const QString &apiKey, QString *error)
{
#ifdef Q_OS_WIN
    const QByteArray secret = apiKey.toUtf8();
    if (secret.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE) {
        if (error != nullptr) {
            *error = QStringLiteral("API Key is too large for Windows Credential Manager.");
        }
        return false;
    }

    CREDENTIALW credential = {};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(CredentialTargetName);
    credential.CredentialBlobSize = static_cast<DWORD>(secret.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char *>(secret.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.UserName = const_cast<LPWSTR>(CredentialUserName);

    if (!CredWriteW(&credential, 0)) {
        setWindowsError(error, QStringLiteral("CredWriteW"), GetLastError());
        return false;
    }

    return true;
#else
    Q_UNUSED(apiKey);
    setUnsupportedError(error);
    return false;
#endif
}

bool WindowsCredentialStorage::deleteApiKey(QString *error)
{
#ifdef Q_OS_WIN
    if (!CredDeleteW(CredentialTargetName, CRED_TYPE_GENERIC, 0)) {
        const DWORD errorCode = GetLastError();
        if (errorCode == ERROR_NOT_FOUND) {
            return true;
        }

        setWindowsError(error, QStringLiteral("CredDeleteW"), errorCode);
        return false;
    }

    return true;
#else
    setUnsupportedError(error);
    return false;
#endif
}
