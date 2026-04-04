#pragma once

#include <QVariant>
#include <QDateTime>
#include <QString>

namespace rtu::core::iec104 {

// ──────────────────────────────────────────────────────────────
// IEC-104 ASDU type identifiers (IEC 60870-5-101 Table 3)
// Only types actively parsed by IEC104Client are listed.
// ──────────────────────────────────────────────────────────────
enum class IEC104TypeId : int {
    // Process information — in monitor direction
    M_SP_NA_1  =  1,   // Single-Point Information
    M_DP_NA_1  =  3,   // Double-Point Information
    M_ST_NA_1  =  5,   // Step-Position Information
    M_BO_NA_1  =  7,   // Bitstring of 32 bits
    M_ME_NA_1  =  9,   // Measured Value, normalized
    M_ME_NB_1  = 11,   // Measured Value, scaled
    M_ME_NC_1  = 13,   // Measured Value, short float
    M_IT_NA_1  = 15,   // Integrated Totals
    M_SP_TB_1  = 30,   // Single-Point with CP56Time2a
    M_DP_TB_1  = 31,   // Double-Point with CP56Time2a
    M_ME_TD_1  = 34,   // Normalized value with CP56Time2a
    M_ME_TE_1  = 35,   // Scaled value with CP56Time2a
    M_ME_TF_1  = 36,   // Short float with CP56Time2a
    // System information — in monitor direction
    M_EI_NA_1  = 70,   // End of Initialization
    // Process information — in control direction
    C_SC_NA_1  = 45,   // Single Command
    C_DC_NA_1  = 46,   // Double Command
    C_RC_NA_1  = 47,   // Regulating Step Command
    C_SE_NA_1  = 48,   // Set-Point Command, normalized
    C_SE_NB_1  = 49,   // Set-Point Command, scaled
    C_SE_NC_1  = 50,   // Set-Point Command, short float
    // System information — in control direction
    C_IC_NA_1  = 100,  // Interrogation Command
    C_CI_NA_1  = 101,  // Counter Interrogation Command
    C_CS_NA_1  = 103,  // Clock Synchronization Command
    // Unknown
    Unknown    = -1
};

// ──────────────────────────────────────────────────────────────
// IEC104DataPoint
//
// Internal representation of one Information Object received
// from an outstation. Carried by the dataPointReceived signal.
// ──────────────────────────────────────────────────────────────
struct IEC104DataPoint {
    int          commonAddr  { 0 };       // ASDU common address
    int          ioa         { 0 };       // Information Object Address
    IEC104TypeId typeId      { IEC104TypeId::Unknown };
    int          cot         { 0 };       // Cause of Transmission
    QVariant     value;                   // bool / int / double
    int          quality     { 0 };       // Quality descriptor bits
    QDateTime    timestamp;               // Local receive time (or CP56Time2a if present)
    bool         hasTimestamp { false };  // true when CP56Time2a was in the ASDU

    // Convenience: human-readable type name
    QString typeName() const;
};

inline QString IEC104DataPoint::typeName() const
{
    switch (typeId) {
        case IEC104TypeId::M_SP_NA_1:  return QStringLiteral("M_SP_NA_1");
        case IEC104TypeId::M_DP_NA_1:  return QStringLiteral("M_DP_NA_1");
        case IEC104TypeId::M_ST_NA_1:  return QStringLiteral("M_ST_NA_1");
        case IEC104TypeId::M_BO_NA_1:  return QStringLiteral("M_BO_NA_1");
        case IEC104TypeId::M_ME_NA_1:  return QStringLiteral("M_ME_NA_1");
        case IEC104TypeId::M_ME_NB_1:  return QStringLiteral("M_ME_NB_1");
        case IEC104TypeId::M_ME_NC_1:  return QStringLiteral("M_ME_NC_1");
        case IEC104TypeId::M_IT_NA_1:  return QStringLiteral("M_IT_NA_1");
        case IEC104TypeId::M_SP_TB_1:  return QStringLiteral("M_SP_TB_1");
        case IEC104TypeId::M_DP_TB_1:  return QStringLiteral("M_DP_TB_1");
        case IEC104TypeId::M_ME_TD_1:  return QStringLiteral("M_ME_TD_1");
        case IEC104TypeId::M_ME_TE_1:  return QStringLiteral("M_ME_TE_1");
        case IEC104TypeId::M_ME_TF_1:  return QStringLiteral("M_ME_TF_1");
        case IEC104TypeId::M_EI_NA_1:  return QStringLiteral("M_EI_NA_1");
        case IEC104TypeId::C_IC_NA_1:  return QStringLiteral("C_IC_NA_1");
        case IEC104TypeId::C_SC_NA_1:  return QStringLiteral("C_SC_NA_1");
        case IEC104TypeId::C_DC_NA_1:  return QStringLiteral("C_DC_NA_1");
        case IEC104TypeId::C_SE_NA_1:  return QStringLiteral("C_SE_NA_1");
        case IEC104TypeId::C_SE_NB_1:  return QStringLiteral("C_SE_NB_1");
        case IEC104TypeId::C_SE_NC_1:  return QStringLiteral("C_SE_NC_1");
        default:                        return QStringLiteral("Unknown(%1)").arg(static_cast<int>(typeId));
    }
}

} // namespace rtu::core::iec104

// Make IEC104DataPoint usable as a Q_DECLARE_METATYPE in signals
Q_DECLARE_METATYPE(rtu::core::iec104::IEC104DataPoint)
