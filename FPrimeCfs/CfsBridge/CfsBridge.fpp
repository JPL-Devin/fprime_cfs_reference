module FPrimeCfs {
    @ Bridge F Prime constructs to the cFS bus
    passive component CfsBridge {
        # This bridge component replaces the "radio" in a traditional F Prime deployment. It connects to the
        # communication stack and takes those buffers in/out of the cFS bus.
        import Svc.Com
    }
}