"""test_fprime_cfs_reference.py:

Basic integration tests for the F Prime CFS reference. Exercises command dispatch,
event generation, and telemetry through the FPrimeDeployment application running
as a CFS application.

Component instance names are resolved through the deployment configuration file
(int_config.json) supplied via the --deployment-config pytest flag.
"""


def test_send_command(fprime_test_api):
    """Test that commands may be sent

    Tests command send, dispatch, and receipt using send_and_assert_command with a
    pair of CommandDispatcher commands.
    """
    cmd_disp = fprime_test_api.get_mnemonic("Svc.CommandDispatcher")
    fprime_test_api.send_and_assert_command(f"{cmd_disp}.CMD_NO_OP", max_delay=5)
    fprime_test_api.send_and_assert_command(
        f"{cmd_disp}.CMD_NO_OP_STRING", ["hello_cfs"], max_delay=5
    )


def test_send_command_args(fprime_test_api):
    """Test that commands with arguments may be sent"""
    cmd_disp = fprime_test_api.get_mnemonic("Svc.CommandDispatcher")
    fprime_test_api.send_and_assert_command(
        f"{cmd_disp}.CMD_TEST_CMD_1", [17, 3, 49], max_delay=5
    )


def test_commands_dispatched_telemetry(fprime_test_api):
    """Test that the CommandsDispatched counter increments after a command"""
    cmd_disp = fprime_test_api.get_mnemonic("Svc.CommandDispatcher")
    result = fprime_test_api.send_and_await_telemetry(
        f"{cmd_disp}.CMD_NO_OP",
        channels=[f"{cmd_disp}.CommandsDispatched"],
        timeout=10,
    )
    assert len(result) > 0, "Did not receive CommandsDispatched telemetry"
    assert result[0].get_val() > 0, "CommandsDispatched did not increment"


def test_event_generation(fprime_test_api):
    """Test that a NO_OP command produces the NoOpReceived event"""
    cmd_disp = fprime_test_api.get_mnemonic("Svc.CommandDispatcher")
    events = [fprime_test_api.get_event_pred(f"{cmd_disp}.NoOpReceived")]
    fprime_test_api.send_and_assert_event(
        f"{cmd_disp}.CMD_NO_OP", events=events, timeout=10
    )


def test_version_command(fprime_test_api):
    """Test that the VERSION command reports framework and project versions"""
    version = fprime_test_api.get_mnemonic("Svc.Version")
    fprime_test_api.send_and_assert_command(f"{version}.VERSION", ["ALL"], max_delay=5)
