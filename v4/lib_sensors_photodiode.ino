// Used for photodiode sensor to convert output value 0-4095 to LUX
float GetLightValue(float AoValue)
{
    float x = 4095.0f - AoValue;

    if (x <= 0.0)
    {
        return 6.0;   // điểm F
    }
    else if (x <= 1051.0)
    {
        // f: DuongThang(F, B)
        return 0.116079923882f * x + 6.0f;
    }
    else if (x <= 3715.0)
    {
        // g: DuongThang(B, C)
        return 0.2394894894895f * x - 123.7034534534535f;
    }
    else if (x <= 3918.68421052632f)
    {
        // h: DuongThang(C, E)
        return 85.935960591133f * x - 318486.09359605913f;
    }
    else if (x <= 3941.0)
    {
        // i: DuongThang(E, D)
        return 634.2608695652174f * x - 2466823.086956522f;
    }
    else
    {
        // j: DuongThang(D, A)
        return 1585.7428571428572f * x - 6216613.6f;
    }
}