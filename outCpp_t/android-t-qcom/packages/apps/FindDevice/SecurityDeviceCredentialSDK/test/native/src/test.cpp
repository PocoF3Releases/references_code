#include <stdio.h>

#include <SecurityDeviceCredentialManager.h>

void test() {
    bool rst = false;
    printf("%d\n", SecurityDeviceCredentialManager::isThisDeviceSupported(&rst));
    printf("%d\n", (int)rst);

    printf("\n");

    const char * rstStr;
    printf("%d\n", SecurityDeviceCredentialManager::getSecurityDeviceId(&rstStr));
    printf("%s\n", rstStr ? rstStr : "N/A");

    printf("\n");

    const unsigned char * signature;
    int signatureLen;
    printf("%d\n", SecurityDeviceCredentialManager::sign(
        0, (const unsigned char *)"\x01\x02\x03", 3, &signature, &signatureLen, false));
    printf("%d\n", signatureLen);
    for (int i = 0; i < signatureLen; i++)
    {
        printf("%02X", signature[i]);
    }

    printf("\n\n");

    printf("%d\n", SecurityDeviceCredentialManager::sign(
            1, (const unsigned char *)"\x01\x02\x03", 3, &signature, &signatureLen, true));
    printf("%d\n", signatureLen);
    for (int i = 0; i < signatureLen; i++)
    {
        printf("%02X", signature[i]);
    }

    printf("\n\n");

    printf("%d\n", SecurityDeviceCredentialManager::signWithDeviceCredential(
            (const unsigned char *)"\x01\x02\x03", 3, &signature, &signatureLen));
    printf("%d\n", signatureLen);
    for (int i = 0; i < signatureLen; i++)
    {
        printf("%02X", signature[i]);
    }

    printf("\n\n");

    printf("%d\n", SecurityDeviceCredentialManager::signWithDeviceCredential(
            (const unsigned char *)"\x01\x02\x03", 3, &signature, &signatureLen, true));
    printf("%d\n", signatureLen);
    for (int i = 0; i < signatureLen; i++)
    {
        printf("%02X", signature[i]);
    }

    printf("\n\n");

    printf("%d\n", SecurityDeviceCredentialManager::signWithFinancialCredential(
            (const unsigned char *)"\x01\x02\x03", 3, &signature, &signatureLen));
    printf("%d\n", signatureLen);
    for (int i = 0; i < signatureLen; i++)
    {
        printf("%02X", signature[i]);
    }

    printf("\n\n");

    printf("%d\n", SecurityDeviceCredentialManager::signWithFinancialCredential(
            (const unsigned char *)"\x01\x02\x03", 3, &signature, &signatureLen, true));
    printf("%d\n", signatureLen);
    for (int i = 0; i < signatureLen; i++)
    {
        printf("%02X", signature[i]);
    }

    printf("\n\n");

    printf("%d\n", SecurityDeviceCredentialManager::forceReload());

    printf("\n");

}

int main() {
    test();
    printf("DONE!\n");
}