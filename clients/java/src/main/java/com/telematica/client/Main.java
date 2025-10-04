package com.telematica.client;

import javax.swing.SwingUtilities;

public class Main {
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> new AppWindow().setVisible(true));
    }
}
