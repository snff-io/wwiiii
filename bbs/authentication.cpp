
//login

static bool VerifyPassword(const std::string& remote_password) {
  a()->UpdateTopScreen();

  if (!remote_password.empty() && remote_password == a()->user()->password()) {
    return true;
  }

  const auto password = bin.input_password(bout.lang().value("PW_PROMPT"), 8);
  return password == a()->user()->password();
}
//TODO: Add OTP verification here
static bool VerifySysopPassword() {
  const auto password = bin.input_password("SY: ", 20);
  return password == a()->config()->system_password();
}

static void DoFailedLoginAttempt() {
  a()->user()->increment_illegal_logons();
  a()->WriteCurrentUser();
  bout.outstr("\r\n\aILLEGAL LOGON\a\r\n\n");

  sysoplog(false, "");
  sysoplog(false, fmt::format("### ILLEGAL LOGON for {}", a()->user()->name_and_number()));
  sysoplog(false, "");
  a()->sess().user_num(0);
}

static void LeaveBadPasswordFeedback(int ans) {
  auto at_exit = finally([] {
    a()->sess().user_num(0);
  });
  
  if (ans > 0) {
    a()->user()->set_flag(User::flag_ansi);
  } else {
    a()->user()->clear_flag(User::flag_ansi);
  }
  bout.outstr("|#6Too many logon attempts!!\r\n\n");
  bout.print("|#9Would you like to leave Feedback to {}? ", a()->config()->sysop_name());
  if (!bin.yesno()) {
    return;
  }
  bout.nl();
  bout.outstr("What is your NAME or HANDLE? ");
  const auto temp_name = bin.input_proper("", 31);
  if (temp_name.empty()) {
    return;
  }
  bout.nl();
  a()->sess().user_num(1);
  a()->user()->set_name(temp_name);
  a()->user()->macro(0, "");
  a()->user()->macro(1, "");
  a()->user()->macro(2, "");
  a()->user()->sl(a()->config()->newuser_sl());
  a()->user()->screen_width(80);
  if (ans > 0) {
    select_editor();
  } else {
    a()->user()->default_editor(0);
  }
  a()->user()->email_sent(0);
  const auto save_allow_cc = a()->IsCarbonCopyEnabled();
  a()->SetCarbonCopyEnabled(false);
  const auto title = StrCat("** Illegal logon feedback from ", temp_name);
  email(title, 1, 0, true, 0, true);
  a()->SetCarbonCopyEnabled(save_allow_cc);
  if (a()->user()->email_sent() > 0) {
    ssm(1) << "Check your mailbox.  Someone forgot their password again!";
  }
}


// existing user 
void change_password() {
  bout.nl();
  bout.outstr("|#9Change password? ");
  if (!bin.yesno()) {
    return;
  }

  bout.nl();
  std::string password = bin.input_password("|#9You must now enter your current password.\r\n|#7: ", 8);
  if (password != a()->user()->password()) {
    bout.outstr("\r\nIncorrect.\r\n\n");
    return;
  }
  bout.nl(2);
  password = bin.input_password("|#9Enter your new password, 3 to 8 characters long.\r\n|#7: ", 8);
  bout.nl(2);
  std::string password2 = bin.input_password("|#9Repeat password for verification.\r\n|#7: ", 8);
  if (password == password2) {
    if (password2.length() < 3) {
      bout.nl();
      bout.outstr("|#6Password must be 3-8 characters long.\r\n|#6Password was not changed.\r\n\n");
    } else {
      a()->user()->password(password);
      bout.outstr("\r\n|#1Password changed.\r\n\n");
      sysoplog("Changed Password.");
    }
  } else {
    bout.outstr("\r\n|#6VERIFY FAILED.\r\n|#6Password not changed.\r\n\n");
  }
}

// ? Use OTP or Password



// Exisiting User