#include "widget.hpp"

#include <QAbstractAnimation>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QSequentialAnimationGroup>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVector>

// Small local "tip" cards shown in a rotating carousel inside the dock.
// These are purely informational -- no network calls, no outbound links.
static QFrame *make_tip_card(QWidget *parent, const QString &emoji, const QString &title, const QString &body)
{
	auto *card = new QFrame(parent);
	card->setObjectName(QStringLiteral("tipCard"));
	card->setFrameShape(QFrame::NoFrame);
	card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	auto *lay = new QHBoxLayout(card);
	lay->setContentsMargins(12, 10, 12, 10);
	lay->setSpacing(10);

	auto *icon = new QLabel(card);
	icon->setText(emoji);
	icon->setStyleSheet(QStringLiteral("font-size:26px;"));

	auto *text = new QLabel(card);
	text->setTextFormat(Qt::RichText);
	text->setWordWrap(true);
	text->setText(QStringLiteral("<b>%1</b><br>%2").arg(title, body));

	lay->addWidget(icon, 0, Qt::AlignVCenter);
	lay->addWidget(text, 1);

	card->setStyleSheet("#tipCard {"
			     "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
			     "    stop:0 #202225, stop:1 #16181a);"
			     "  border:1px solid #33363a; border-radius:10px; padding:6px; }"
			     "#tipCard QLabel { color:#f2f2f2; }");

	return card;
}

QWidget *widget_create_tip_card_a(QWidget *parent)
{
	return make_tip_card(parent, QString::fromUtf8("\xF0\x9F\x8E\xAC"), QObject::tr("Quick tip"),
			      QObject::tr("Add a logo or profile photo to any lower third from the "
					  "template editor -- it scales automatically with the layout."));
}

QWidget *widget_create_tip_card_b(QWidget *parent)
{
	return make_tip_card(parent, QString::fromUtf8("\xE2\x9C\xA8"), QObject::tr("Quick tip"),
			      QObject::tr("Try a different entrance/exit animation per template -- mixing "
					  "styles across a broadcast keeps things feeling alive."));
}

QWidget *create_widget_carousel(QWidget *parent)
{
	auto *wrapper = new QWidget(parent);
	wrapper->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	auto *root = new QVBoxLayout(wrapper);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(4);

	auto *stack = new QStackedWidget(wrapper);
	stack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	stack->addWidget(widget_create_tip_card_a(stack));
	stack->addWidget(widget_create_tip_card_b(stack));
	stack->addWidget(make_tip_card(stack, QString::fromUtf8("\xE2\x8C\xA8"), QObject::tr("Quick tip"),
					QObject::tr("Assign a hotkey to any lower third for instant recall "
						    "during a live show.")));

	root->addWidget(stack);

	QVector<QToolButton *> dots;
	auto *dotsRow = new QHBoxLayout();
	dotsRow->setContentsMargins(0, 0, 0, 0);
	dotsRow->setSpacing(6);
	dotsRow->addStretch(1);

	const int count = stack->count();
	for (int i = 0; i < count; ++i) {
		auto *dot = new QToolButton(wrapper);
		dot->setText(QStringLiteral("\xE2\x97\x8F"));
		dot->setCheckable(true);
		dot->setAutoExclusive(true);
		dot->setCursor(Qt::PointingHandCursor);
		dot->setToolTip(QStringLiteral("Show tip %1").arg(i + 1));
		dot->setStyleSheet("QToolButton {"
				   "  border: none;"
				   "  background: transparent;"
				   "  color: #666666;"
				   "  font-size: 13px;"
				   "  padding: 0;"
				   "}"
				   "QToolButton:checked {"
				   "  color: #ffffff;"
				   "}");
		dots << dot;
		dotsRow->addWidget(dot);
	}
	dotsRow->addStretch(1);
	root->addLayout(dotsRow);

	auto *effect = new QGraphicsOpacityEffect(stack);
	effect->setOpacity(1.0);
	stack->setGraphicsEffect(effect);

	auto updateDots = [stack, dots]() {
		const int idx = stack->currentIndex();
		for (int i = 0; i < dots.size(); ++i) {
			if (dots[i])
				dots[i]->setChecked(i == idx);
		}
	};

	updateDots();

	auto *timer = new QTimer(wrapper);
	timer->setInterval(20000);

	auto switchToIndex = [stack, effect, updateDots, wrapper](int targetIndex) {
		if (targetIndex < 0 || targetIndex >= stack->count())
			return;
		if (targetIndex == stack->currentIndex())
			return;

		auto *fadeOut = new QPropertyAnimation(effect, "opacity", wrapper);
		fadeOut->setDuration(180);
		fadeOut->setStartValue(1.0);
		fadeOut->setEndValue(0.0);

		auto *fadeIn = new QPropertyAnimation(effect, "opacity", wrapper);
		fadeIn->setDuration(180);
		fadeIn->setStartValue(0.0);
		fadeIn->setEndValue(1.0);

		auto *group = new QSequentialAnimationGroup(wrapper);
		group->addAnimation(fadeOut);
		group->addAnimation(fadeIn);

		QObject::connect(fadeOut, &QPropertyAnimation::finished, stack, [stack, targetIndex, updateDots]() {
			stack->setCurrentIndex(targetIndex);
			updateDots();
		});

		group->start(QAbstractAnimation::DeleteWhenStopped);
	};

	for (int i = 0; i < dots.size(); ++i) {
		if (!dots[i])
			continue;

		QObject::connect(dots[i], &QToolButton::clicked, wrapper, [i, timer, switchToIndex]() {
			if (timer)
				timer->start();
			switchToIndex(i);
		});
	}

	QObject::connect(timer, &QTimer::timeout, wrapper, [stack, switchToIndex]() {
		if (stack->count() == 0)
			return;
		int next = stack->currentIndex() + 1;
		if (next >= stack->count())
			next = 0;
		switchToIndex(next);
	});

	timer->start();
	return wrapper;
}

static QFrame *make_help_card(QWidget *parent, const QString &emoji, const QString &title, const QString &body)
{
	auto *card = new QFrame(parent);
	card->setObjectName(QStringLiteral("ocHelpCard"));
	card->setFrameShape(QFrame::NoFrame);

	auto *lay = new QHBoxLayout(card);
	lay->setContentsMargins(12, 10, 12, 10);
	lay->setSpacing(10);

	auto *ico = new QLabel(card);
	ico->setText(emoji);
	ico->setMinimumWidth(34);
	ico->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	ico->setStyleSheet(QStringLiteral("font-size:22px;"));

	auto *textCol = new QVBoxLayout();
	textCol->setContentsMargins(0, 0, 0, 0);
	textCol->setSpacing(2);

	auto *t = new QLabel(card);
	t->setText(QStringLiteral("<b>%1</b>").arg(title));
	t->setTextFormat(Qt::RichText);

	auto *sub = new QLabel(card);
	sub->setText(body);
	sub->setWordWrap(true);
	sub->setStyleSheet(QStringLiteral("color: rgba(255,255,255,0.75);"));

	textCol->addWidget(t);
	textCol->addWidget(sub);

	lay->addWidget(ico, 0, Qt::AlignTop);
	lay->addLayout(textCol, 1);

	return card;
}

// Self-contained help dialog: local troubleshooting guidance only.
// No outbound links, no network calls.
void show_troubleshooting_dialog(QWidget *parent)
{
	auto *dlg = new QDialog(parent);
	dlg->setAttribute(Qt::WA_DeleteOnClose, true);
	dlg->setWindowTitle(QObject::tr("MediaScope Lower Thirds \xE2\x80\xA2 Help"));
	dlg->setModal(true);
	dlg->resize(680, 520);

	auto *root = new QVBoxLayout(dlg);
	root->setContentsMargins(14, 14, 14, 14);
	root->setSpacing(10);

	{
		auto *hdr = new QFrame(dlg);
		hdr->setObjectName(QStringLiteral("ocHelpHeader"));
		hdr->setFrameShape(QFrame::NoFrame);
		auto *hl = new QVBoxLayout(hdr);
		hl->setContentsMargins(14, 12, 14, 12);
		hl->setSpacing(6);

		auto *title = new QLabel(QObject::tr("Troubleshooting"), hdr);
		title->setStyleSheet(QStringLiteral("font-size:16px; font-weight:700;"));

		auto *desc = new QLabel(
			QObject::tr("If changes don't apply in OBS, confirm your Resources output folder "
				    "has read/write access (a Documents subfolder is recommended), and that "
				    "the Browser Source is pointed at the generated HTML file."),
			hdr);
		desc->setWordWrap(true);
		desc->setStyleSheet(QStringLiteral("color: rgba(255,255,255,0.78);"));

		hl->addWidget(title);
		hl->addWidget(desc);
		root->addWidget(hdr);
	}

	auto *scroll = new QScrollArea(dlg);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);

	auto *content = new QWidget(scroll);
	auto *cl = new QVBoxLayout(content);
	cl->setContentsMargins(0, 0, 0, 0);
	cl->setSpacing(10);

	{
		auto *secTitle = new QLabel(QObject::tr("Common fixes"), content);
		secTitle->setStyleSheet(QStringLiteral("font-weight:700;"));
		cl->addWidget(secTitle);

		cl->addWidget(make_help_card(
			content, QString::fromUtf8("\xF0\x9F\x94\x84"), QObject::tr("Overlay not updating"),
			QObject::tr("Right-click the Browser Source in OBS and choose \"Refresh cache of "
				    "current page\", or toggle the source's visibility off and on.")));

		cl->addWidget(make_help_card(
			content, QString::fromUtf8("\xF0\x9F\x93\x81"), QObject::tr("Nothing shows up at all"),
			QObject::tr("Check the Resources path in Settings -- it must point to a writable "
				    "folder, and the Browser Source URL must match the generated "
				    "index.html exactly.")));

		cl->addWidget(make_help_card(
			content, QString::fromUtf8("\xF0\x9F\x94\xA4"), QObject::tr("Font or logo not applying"),
			QObject::tr("Custom fonts must be installed system-wide to appear in the font "
				    "picker. Logos/profile images should be PNG or JPG under 5MB for the "
				    "fastest load.")));

		cl->addWidget(make_help_card(
			content, QString::fromUtf8("\xF0\x9F\x8E\x9E"), QObject::tr("Animations look choppy"),
			QObject::tr("Match the OBS Browser Source's custom FPS to your canvas FPS, and "
				    "enable hardware acceleration if available.")));
	}

	cl->addStretch(1);
	scroll->setWidget(content);
	root->addWidget(scroll, 1);

	{
		auto *bb = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
		QObject::connect(bb, &QDialogButtonBox::rejected, dlg, &QDialog::close);
		root->addWidget(bb);
	}

	dlg->setStyleSheet(
		"#ocHelpHeader {"
		"  background: rgba(255,255,255,0.06);"
		"  border: 1px solid rgba(255,255,255,0.10);"
		"  border-radius: 12px;"
		"}"
		"QScrollArea { background: transparent; }"
		"QDialog { background: #141416; color: white; }"
		"#ocHelpCard {"
		"  background: rgba(255,255,255,0.05);"
		"  border: 1px solid rgba(255,255,255,0.10);"
		"  border-radius: 12px;"
		"}"
		"#ocHelpCard:hover { border-color: rgba(255,255,255,0.18); }"
	);

	dlg->show();
}
