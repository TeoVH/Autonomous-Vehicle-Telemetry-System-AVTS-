package com.telematica.client;

import javax.swing.*;
import java.awt.*;
import java.util.Map;

public class AppWindow extends JFrame {
    private final JTextField hostField = new JTextField("127.0.0.1");
    private final JSpinner  portField = new JSpinner(new SpinnerNumberModel(5050, 1, 65535, 1));
    private final JTextField roleField = new JTextField("ADMIN");
    private final JTextField userField = new JTextField("root");
    private final JPasswordField passField = new JPasswordField("password");
    private final JCheckBox demoMode = new JCheckBox("Demo sin servidor");

    private final JButton btnLogin = new JButton("Login");

    // Botones vigentes
    private final JButton btnStart       = new JButton("START");        // alias a SPEED UP
    private final JButton btnTurnLeft    = new JButton("TURN LEFT");
    private final JButton btnTurnRight   = new JButton("TURN RIGHT");
    private final JButton btnBatteryReset= new JButton("BATTERY RESET");

    private final JLabel speedLbl   = new JLabel("—");
    private final JLabel batteryLbl = new JLabel("—");
    private final JLabel dirLbl     = new JLabel("—");
    private final JLabel tempLbl    = new JLabel("—");

    // Estado último comando (OK/ERR/…)
    private final JLabel cmdStatusTitle = new JLabel("Último comando:");
    private final JLabel cmdStatusLbl   = new JLabel("—");

    private final JTextArea logArea = new JTextArea(8, 60);

    private TelemetryClient client;

    public AppWindow() {
        super("Telemática — Java Client (Swing)");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(820, 560);
        setLocationRelativeTo(null);

        logArea.setEditable(false);
        setLayout(new BorderLayout(10, 10));
        add(buildTopPanel(), BorderLayout.NORTH);
        add(buildCenterPanel(), BorderLayout.CENTER);
        add(new JScrollPane(logArea), BorderLayout.SOUTH);

        setControlsEnabled(false); // hasta que el login sea OK
        wireEvents();
    }

    private JPanel buildTopPanel() {
        var p = new JPanel(new GridBagLayout());
        var c = new GridBagConstraints();
        c.insets = new Insets(4,4,4,4);
        c.fill = GridBagConstraints.HORIZONTAL;

        int y = 0;
        c.gridx = 0; c.gridy = y; p.add(new JLabel("Host"), c);
        c.gridx = 1; c.gridy = y; p.add(hostField, c);
        c.gridx = 2; c.gridy = y; p.add(new JLabel("Puerto"), c);
        c.gridx = 3; c.gridy = y; p.add(portField, c);
        y++;
        c.gridx = 0; c.gridy = y; p.add(new JLabel("Role"), c);
        c.gridx = 1; c.gridy = y; p.add(roleField, c);
        c.gridx = 2; c.gridy = y; p.add(new JLabel("Usuario"), c);
        c.gridx = 3; c.gridy = y; p.add(userField, c);
        y++;
        c.gridx = 0; c.gridy = y; p.add(new JLabel("Password"), c);
        c.gridx = 1; c.gridy = y; p.add(passField, c);
        c.gridx = 2; c.gridy = y; p.add(demoMode, c);
        c.gridx = 3; c.gridy = y; p.add(btnLogin, c);

        return p;
    }

    private JPanel buildCenterPanel() {
        var p = new JPanel(new GridBagLayout());
        var c = new GridBagConstraints();
        c.insets = new Insets(8,8,8,8);
        c.fill = GridBagConstraints.HORIZONTAL;

        int y = 0;
        // Telemetría
        c.gridx = 0; c.gridy = y; p.add(new JLabel("Speed"), c);
        c.gridx = 1; c.gridy = y; p.add(speedLbl, c);
        c.gridx = 2; c.gridy = y; p.add(new JLabel("Battery"), c);
        c.gridx = 3; c.gridy = y; p.add(batteryLbl, c);
        y++;
        c.gridx = 0; c.gridy = y; p.add(new JLabel("Dir"), c);
        c.gridx = 1; c.gridy = y; p.add(dirLbl, c);
        c.gridx = 2; c.gridy = y; p.add(new JLabel("Temp"), c);
        c.gridx = 3; c.gridy = y; p.add(tempLbl, c);
        y++;

        // Fila de comandos
        c.gridx = 0; c.gridy = y; p.add(btnStart, c);           // START -> SPEED UP
        c.gridx = 1; c.gridy = y; p.add(btnTurnLeft, c);
        c.gridx = 2; c.gridy = y; p.add(btnTurnRight, c);
        c.gridx = 3; c.gridy = y; p.add(btnBatteryReset, c);
        y++;

        // Estado último comando
        c.gridx = 0; c.gridy = y; p.add(cmdStatusTitle, c);
        c.gridx = 1; c.gridy = y; c.gridwidth = 3; p.add(cmdStatusLbl, c);
        c.gridwidth = 1;

        return p;
    }

    private void wireEvents() {
        btnLogin.addActionListener(e -> onLogin());

        // START mapeado a SPEED UP
        btnStart.addActionListener(e -> sendCommandAliasStart());

        // Botones nativos del servidor
        btnTurnLeft.addActionListener(e  -> sendWire("TURN LEFT"));
        btnTurnRight.addActionListener(e -> sendWire("TURN RIGHT"));
        btnBatteryReset.addActionListener(e -> sendWire("BATTERY RESET"));
    }

    private void onLogin() {
        if (client != null && client.isConnected()) {
            appendLog("Ya conectado.");
            return;
        }
        btnLogin.setEnabled(false);

        if (demoMode.isSelected()) {
            appendLog("Modo Demo: generando DATA cada 1s");
            setControlsEnabled(true);
            startDemo();
            return;
        }

        String host = hostField.getText().trim();
        int port = (Integer) portField.getValue();
        String role = roleField.getText().trim();
        String user = userField.getText().trim();
        String pass = new String(passField.getPassword());

        client = new TelemetryClient(host, port, role, user, pass, new TelemetryClient.Listener() {
            @Override public void onConnected() { appendLog("Conectado a " + host + ":" + port); }
            @Override public void onLoginResult(boolean ok, String line) {
                appendLog("LOGIN → " + line);
                setControlsEnabled(ok);
                if (!ok) btnLogin.setEnabled(true);
            }
            @Override public void onStatus(Map<String, String> kv) { updateStatus(kv); }
            @Override public void onServerMessage(String line) {
                if (line == null || line.trim().isEmpty()) return; // filtra líneas vacías
                appendLog("SRV: " + line);
                String u = line.trim().toUpperCase();
                if (u.startsWith("OK")) setCmdStatus("OK");
                else if (u.startsWith("ERR")) setCmdStatus("ERR");
            }
            @Override public void onError(String msg, Exception ex) {
                appendLog("ERR: " + msg + (ex!=null?" — "+ex.getMessage():""));
                btnLogin.setEnabled(true);
                setControlsEnabled(false);
            }
            @Override public void onClosed() {
                appendLog("Conexión cerrada");
                btnLogin.setEnabled(true);
                setControlsEnabled(false);
            }
        });

        client.connectAsync();
    }

    private void startDemo() {
        new Thread(() -> {
            double speed = 0;
            double temp = 25;
            int battery = 100;
            String[] dirs = {"FORWARD","LEFT","RIGHT","STOP"};
            int dIdx = 0;

            try {
                while (true) {
                    speed = (speed + 3) % 120;
                    temp += (Math.random() - 0.5);
                    battery = Math.max(0, battery - (int)(Math.random()*2));
                    dIdx = (dIdx + 1) % dirs.length;

                    var kv = Map.of(
                        "speed", String.format("%.1f", speed),
                        "battery", String.valueOf(battery),
                        "dir", dirs[dIdx],
                        "temp", String.format("%.1f", temp)
                    );
                    SwingUtilities.invokeLater(() -> updateStatus(kv));
                    Thread.sleep(1000);
                }
            } catch (InterruptedException ignored) {}
        }, "demo-loop").start();
    }

    // START → SPEED UP
    private void sendCommandAliasStart() {
        if (demoMode.isSelected()) {
            setCmdStatus("OK (demo)");
            appendLog("CMD (demo): START → OK");
            return;
        }
        if (client == null || !client.isConnected()) { appendLog("No conectado"); return; }
        client.send("SPEED UP\n");
        appendLog("CMD: SPEED UP");
    }

    // Enviar comandos nativos tal cual
    private void sendWire(String wire) {
        if (demoMode.isSelected()) {
            setCmdStatus("OK (demo)");
            appendLog("CMD (demo): " + wire + " → OK");
            return;
        }
        if (client == null || !client.isConnected()) { appendLog("No conectado"); return; }
        client.send(wire + "\n");
        appendLog("CMD: " + wire);
    }

    private void setControlsEnabled(boolean enabled) {
        btnStart.setEnabled(enabled);
        btnTurnLeft.setEnabled(enabled);
        btnTurnRight.setEnabled(enabled);
        btnBatteryReset.setEnabled(enabled);
    }

    private void setCmdStatus(String s) { cmdStatusLbl.setText(s); }

    private void updateStatus(Map<String, String> kv) {
        speedLbl.setText(kv.getOrDefault("speed", "—"));
        batteryLbl.setText(kv.getOrDefault("battery", "—"));
        dirLbl.setText(kv.getOrDefault("dir", "—"));
        tempLbl.setText(kv.getOrDefault("temp", "—"));
    }

    private void appendLog(String s) {
        SwingUtilities.invokeLater(() -> {
            logArea.append(s + "\n");
            logArea.setCaretPosition(logArea.getDocument().getLength());
        });
    }
}
