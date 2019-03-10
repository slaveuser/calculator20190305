// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scifunc.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    SciCalcFunctions--do sin, cos, tan, com, log, ln, rec, fac, etc.***/
/***    DisplayError--Error display driver.                             ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    SciCalcFunctions call DisplayError.                             ***/
/***                                                                    ***/
/***                                                                    ***/
/**************************************************************************/
#include "pch.h"
#include "Header Files/CalcEngine.h"

using namespace std;
using namespace CalcEngine;
using namespace CalcEngine::RationalMath;

/* Routines for more complex mathematical functions/error checking. */
CalcEngine::Rational CCalcEngine::SciCalcFunctions(CalcEngine::Rational const& rat, DWORD op)
{
    Rational result{};
    try
    {
        switch (op)
        {
        case IDC_CHOP:
            result = m_bInv ? Frac(rat) : Integer(rat);
            break;

            /* Return complement. */
        case IDC_COM:
            if (m_radix == 10 && !m_fIntegerMode)
            {
                result = -(RationalMath::Integer(rat) + 1);
            }
            else
            {
                result = rat ^ m_chopNumbers[m_numwidth];
            }
            break;

            // Rotate Left with hi bit wrapped over to lo bit
        case IDC_ROL:
            if (m_fIntegerMode)
            {
                result = Integer(rat);

                uint64_t w64Bits = result.ToUInt64_t();
                uint64_t msb = (w64Bits >> (m_dwWordBitWidth - 1)) & 1;
                w64Bits <<= 1; // LShift by 1
                w64Bits |= msb; // Set the prev Msb as the current Lsb

                result = w64Bits;
            }
            break;

            // Rotate right with lo bit wrapped over to hi bit
        case IDC_ROR:
            if (m_fIntegerMode)
            {
                result = Integer(rat);

                uint64_t w64Bits = result.ToUInt64_t();
                uint64_t lsb = ((w64Bits & 0x01) == 1) ? 1 : 0;
                w64Bits >>= 1; //RShift by 1
                w64Bits |= (lsb << (m_dwWordBitWidth - 1));

                result = w64Bits;
            }
            break;

        case IDC_PERCENT:
        {
            // If the operator is multiply/divide, we evaluate this as "X [op] (Y%)"
            // Otherwise, we evaluate it as "X [op] (X * Y%)"
            if (m_nOpCode == IDC_MUL || m_nOpCode == IDC_DIV)
            {
                result = rat / 100;
            }
            else
            {
                result = rat * (m_lastVal / 100);
            }
            break;
        }

        case IDC_SIN: /* Sine; normal and arc */
            if (!m_fIntegerMode)
            {
                result = m_bInv ? ASin(rat, m_angletype) : Sin(rat, m_angletype);
            }
            break;

        case IDC_SINH: /* Sine- hyperbolic and archyperbolic */
            if (!m_fIntegerMode)
            {
                result = m_bInv ? ASinh(rat) : Sinh(rat);
            }
            break;

        case IDC_COS: /* Cosine, follows convention of sine function. */
            if (!m_fIntegerMode)
            {
                result = m_bInv ? ACos(rat, m_angletype) : Cos(rat, m_angletype);
            }
            break;

        case IDC_COSH: /* Cosine hyperbolic, follows convention of sine h function. */
            if (!m_fIntegerMode)
            {
                result = m_bInv ? ACosh(rat) : Cosh(rat);
            }
            break;

        case IDC_TAN: /* Same as sine and cosine. */
            if (!m_fIntegerMode)
            {
                result = m_bInv ? ATan(rat, m_angletype) : Tan(rat, m_angletype);
            }
            break;

        case IDC_TANH: /* Same as sine h and cosine h. */
            if (!m_fIntegerMode)
            {
                result = m_bInv ? ATanh(rat) : Tanh(rat);
            }
            break;

        case IDC_REC: /* Reciprocal. */
            result = Invert(rat);
            break;

        case IDC_SQR: /* Square */
            result = Pow(rat, 2);
            break;

        case IDC_SQRT: /* Square Root */
            result = Root(rat, 2);
            break;

        case IDC_CUBEROOT:
        case IDC_CUB: /* Cubing and cube root functions. */
            result = IDC_CUBEROOT == op ? Root(rat, 3) : Pow(rat, 3);
            break;

        case IDC_LOG: /* Functions for common log. */
            result = Log10(rat);
            break;

        case IDC_POW10:
            result = Pow(10, rat);
            break;

        case IDC_LN: /* Functions for natural log. */
            result = m_bInv ? Exp(rat) : Log(rat);
            break;

        case IDC_FAC: /* Calculate factorial.  Inverse is ineffective. */
            result = Fact(rat);
            break;

        case IDC_DEGREES:
            ProcessCommand(IDC_INV);
            // This case falls through to IDC_DMS case because in the old Win32 Calc, 
            // the degrees functionality was achieved as 'Inv' of 'dms' operation,
            // so setting the IDC_INV command first and then performing 'dms' operation as global variables m_bInv, m_bRecord 
            // are set properly through ProcessCommand(IDC_INV)
        case IDC_DMS:
        {
            if (!m_fIntegerMode)
            {
                auto shftRat{ m_bInv ? 100 : 60 };

                Rational degreeRat = Integer(rat);

                Rational minuteRat = (rat - degreeRat) * shftRat;

                Rational secondRat = minuteRat;

                minuteRat = Integer(minuteRat);

                secondRat = (secondRat - minuteRat) * shftRat;

                //
                // degreeRat == degrees, minuteRat == minutes, secondRat == seconds
                //

                shftRat = m_bInv ? 60 : 100;
                secondRat /= shftRat;

                minuteRat = (minuteRat + secondRat) / shftRat;

                result = degreeRat + minuteRat;
            }
            break;
        }
        }   // end switch( op )
    }
    catch (DWORD nErrCode)
    {
        DisplayError(nErrCode);
        result = rat;
    }

    return result;
}

/* Routine to display error messages and set m_bError flag.  Errors are */
/* called with DisplayError (n), where n is a DWORD   between 0 and 5. */

void CCalcEngine::DisplayError(DWORD nError)
{
    wstring errorString{ GetString(IDS_ERRORS_FIRST + SCODE_CODE(nError)) };

    SetPrimaryDisplay(errorString, true /*isError*/);

    m_bError = true; /* Set error flag.  Only cleared with CLEAR or CENTR. */

    m_HistoryCollector.ClearHistoryLine(errorString);
}
