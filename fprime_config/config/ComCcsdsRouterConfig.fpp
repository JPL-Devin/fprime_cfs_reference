module ComCcsds {

    # Override of the core ComCcsds router configuration: use the cFS-aware
    # FPrimeCfs.CfsRouter in place of the default Svc.FprimeRouter. Routing is
    # driven by the static APID route table in CfsRouterConfig.
    instance fprimeRouter: FPrimeCfs.CfsRouter base id ComCcsdsConfig.BASE_ID + 0x03000
}
