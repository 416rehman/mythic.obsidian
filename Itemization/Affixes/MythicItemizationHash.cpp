#include "Itemization/Affixes/MythicItemizationHash.h"

namespace {
uint32 RotateRight(const uint32 Value, const uint32 Bits) {
    return (Value >> Bits) | (Value << (32u - Bits));
}
}

bool MythicItemizationHash::Sha256(const TConstArrayView<uint8> Bytes,
                                   FSHA256Signature &OutSignature) {
    // FPlatformMisc::GetSHA256Signature is only an optional platform hook and asserts on Windows in UE 5.8.
    // Keep the frozen itemization algorithm project-local so dedicated servers, editor tools and every target use
    // the same standard FIPS 180-4 byte transform without a platform or third-party crypto dependency.
    static constexpr uint32 RoundConstants[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
    };
    uint32 State[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };

    if (Bytes.Num() < 0) {
        return false;
    }
    const uint64 BitLength = static_cast<uint64>(Bytes.Num()) * 8u;
    TArray<uint8> Padded;
    const int32 PaddingBytes = 1 + ((56 - ((Bytes.Num() + 1) % 64) + 64) % 64) + 8;
    Padded.Reserve(Bytes.Num() + PaddingBytes);
    Padded.Append(Bytes.GetData(), Bytes.Num());
    Padded.Add(0x80u);
    while ((Padded.Num() % 64) != 56) {
        Padded.Add(0u);
    }
    for (int32 Shift = 56; Shift >= 0; Shift -= 8) {
        Padded.Add(static_cast<uint8>(BitLength >> Shift));
    }

    for (int32 Offset = 0; Offset < Padded.Num(); Offset += 64) {
        uint32 Schedule[64];
        for (int32 Index = 0; Index < 16; ++Index) {
            const uint8 *Word = Padded.GetData() + Offset + Index * 4;
            Schedule[Index] = (static_cast<uint32>(Word[0]) << 24)
                | (static_cast<uint32>(Word[1]) << 16)
                | (static_cast<uint32>(Word[2]) << 8)
                | static_cast<uint32>(Word[3]);
        }
        for (int32 Index = 16; Index < 64; ++Index) {
            const uint32 S0 = RotateRight(Schedule[Index - 15], 7)
                ^ RotateRight(Schedule[Index - 15], 18) ^ (Schedule[Index - 15] >> 3);
            const uint32 S1 = RotateRight(Schedule[Index - 2], 17)
                ^ RotateRight(Schedule[Index - 2], 19) ^ (Schedule[Index - 2] >> 10);
            Schedule[Index] = Schedule[Index - 16] + S0 + Schedule[Index - 7] + S1;
        }

        uint32 A = State[0], B = State[1], C = State[2], D = State[3];
        uint32 E = State[4], F = State[5], G = State[6], H = State[7];
        for (int32 Index = 0; Index < 64; ++Index) {
            const uint32 S1 = RotateRight(E, 6) ^ RotateRight(E, 11) ^ RotateRight(E, 25);
            const uint32 Choice = (E & F) ^ ((~E) & G);
            const uint32 Temp1 = H + S1 + Choice + RoundConstants[Index] + Schedule[Index];
            const uint32 S0 = RotateRight(A, 2) ^ RotateRight(A, 13) ^ RotateRight(A, 22);
            const uint32 Majority = (A & B) ^ (A & C) ^ (B & C);
            const uint32 Temp2 = S0 + Majority;
            H = G; G = F; F = E; E = D + Temp1;
            D = C; C = B; B = A; A = Temp1 + Temp2;
        }
        State[0] += A; State[1] += B; State[2] += C; State[3] += D;
        State[4] += E; State[5] += F; State[6] += G; State[7] += H;
    }

    for (int32 Index = 0; Index < 8; ++Index) {
        OutSignature.Signature[Index * 4] = static_cast<uint8>(State[Index] >> 24);
        OutSignature.Signature[Index * 4 + 1] = static_cast<uint8>(State[Index] >> 16);
        OutSignature.Signature[Index * 4 + 2] = static_cast<uint8>(State[Index] >> 8);
        OutSignature.Signature[Index * 4 + 3] = static_cast<uint8>(State[Index]);
    }
    return true;
}
